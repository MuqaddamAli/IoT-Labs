#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//-------- Display --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//-------- Smooth PWM Breathe --------
const uint32_t BREATHE_PERIOD_MS = 2500;
const float    GAMMA              = 2.2f;
const uint8_t  MIN_DUTY           = 3;
const uint8_t  MAX_DUTY           = 255;

//-------- Pins --------
const uint8_t LED_RED    = 12;
const uint8_t LED_YELLOW = 14;
const uint8_t LED_GREEN  = 27;
const uint8_t BTN_CYCLE  = 18;
const uint8_t BTN_HOME   = 19;

//-------- LEDC PWM --------
const uint8_t  PWM_RED_CH     = 0;
const uint8_t  PWM_YELLOW_CH  = 1;
const uint8_t  PWM_GREEN_CH   = 2;
const uint32_t PWM_FREQUENCY  = 5000;
const uint8_t  PWM_RESOLUTION = 8;

//-------- Modes --------
enum DisplayMode {
  MODE_SLEEP = 0,
  MODE_DANCE = 1,
  MODE_PARTY = 2,
  MODE_BREATHE = 3
};
DisplayMode currentMode = MODE_SLEEP;

//-------- Flags --------
volatile bool requestModeChange = false;
volatile bool requestReset = false;
volatile uint32_t lastCyclePress = 0;
volatile uint32_t lastHomePress  = 0;

//-------- Timers --------
uint32_t animationTimer = 0;
uint32_t displayRefreshTimer = 0;
bool alternateState = false;

//-------- ASCII emoticons --------
const char* EMOTE_SLEEP   = "Zzz";
const char* EMOTE_DANCE_A = "(^_^)";
const char* EMOTE_DANCE_B = "(-_-)";  
const char* EMOTE_PARTY   = "(*_*)";
const char* EMOTE_BREATHE = "(^_^)";

//-------- Helpers --------
inline float gammaCorrect(float x) {
  if (x < 0.f) x = 0.f;
  if (x > 1.f) x = 1.f;
  return powf(x, GAMMA);
}

void clearAllLEDs() {
  ledcWrite(PWM_RED_CH, 0);
  ledcWrite(PWM_YELLOW_CH, 0);
  ledcWrite(PWM_GREEN_CH, 0);
}

//-------- ISRs --------
void IRAM_ATTR handleCycleButton() {
  uint32_t now = millis();
  if (now - lastCyclePress > 250) {
    requestModeChange = true;
    lastCyclePress = now;
  }
}

void IRAM_ATTR handleHomeButton() {
  uint32_t now = millis();
  if (now - lastHomePress > 250) {
    requestReset = true;
    lastHomePress = now;
  }
}

//-------- Display Update --------
void updateOLED() {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);

  //Title
  display.setTextSize(1);
  display.setCursor(35, 8);
  display.print("~ MODE ~");

  //Mode label
  display.setTextSize(2);
  display.setCursor(15, 24);
  switch(currentMode) {
    case MODE_SLEEP:   display.print("SLEEP");   break;
    case MODE_DANCE:   display.print("BLINK");   break;
    case MODE_PARTY:   display.print("PARTY!");  break;
    case MODE_BREATHE: display.print("CHILL");   break;
  }

  //Emoticon (small side)
  display.setTextSize(1);
  display.setCursor(85, 28);

  switch(currentMode) {
    case MODE_SLEEP:
      display.print(EMOTE_SLEEP);
      break;

    case MODE_DANCE:
      if (alternateState) display.print(EMOTE_DANCE_A);
      else display.print(EMOTE_DANCE_B);
      break;

    case MODE_PARTY:
      display.print(EMOTE_PARTY);
      break;

    case MODE_BREATHE:
      display.print(EMOTE_BREATHE);
      break;
  }

  display.setCursor(5, 50);
  display.print("[");
  display.print((int)currentMode);
  display.print("/3] Press to cycle");

  display.display();
}

void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

  ledcSetup(PWM_RED_CH, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_YELLOW_CH, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_GREEN_CH, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(LED_RED, PWM_RED_CH);
  ledcAttachPin(LED_YELLOW, PWM_YELLOW_CH);
  ledcAttachPin(LED_GREEN, PWM_GREEN_CH);

  pinMode(BTN_CYCLE, INPUT_PULLUP);
  pinMode(BTN_HOME,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_CYCLE), handleCycleButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_HOME),  handleHomeButton, FALLING);

  clearAllLEDs();
  updateOLED();
}

void loop() {
  uint32_t currentTime = millis();

  if (requestReset) {
    currentMode = MODE_SLEEP;
    clearAllLEDs();
    requestReset = false;
  }

  if (requestModeChange) {
    currentMode = static_cast<DisplayMode>((currentMode + 1) % 4);
    clearAllLEDs();
    requestModeChange = false;
  }

  switch(currentMode) {
    case MODE_SLEEP:
      clearAllLEDs();
      break;

    case MODE_DANCE:
      if (currentTime - animationTimer >= 400) {
        animationTimer = currentTime;
        alternateState = !alternateState;
        if (alternateState) {
          ledcWrite(PWM_RED_CH, 255);
          ledcWrite(PWM_YELLOW_CH, 0);
          ledcWrite(PWM_GREEN_CH, 255);
        } else {
          ledcWrite(PWM_RED_CH, 0);
          ledcWrite(PWM_YELLOW_CH, 255);
          ledcWrite(PWM_GREEN_CH, 0);
        }
      }
      break;

    case MODE_PARTY:
      ledcWrite(PWM_RED_CH, 255);
      ledcWrite(PWM_YELLOW_CH, 255);
      ledcWrite(PWM_GREEN_CH, 255);
      break;

    case MODE_BREATHE: {
      uint32_t t = currentTime % BREATHE_PERIOD_MS;
      float phase = (float)t / (float)BREATHE_PERIOD_MS;
      float breath = 0.5f * (1.0f - cosf(2.0f * PI * phase));
      float corrected = gammaCorrect(breath);
      uint8_t duty = (uint8_t)(MIN_DUTY + corrected * (MAX_DUTY - MIN_DUTY) + 0.5f);
      ledcWrite(PWM_RED_CH, duty);
      ledcWrite(PWM_YELLOW_CH, duty);
      ledcWrite(PWM_GREEN_CH, duty);
    } break;
  }

  if (currentTime - displayRefreshTimer >= 100) {
    displayRefreshTimer = currentTime;
    updateOLED();
  }
}

// // //// ---- TASK 2 ----

// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // -------- Display --------
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// #define OLED_ADDRESS 0x3C
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// // -------- Pins --------
// // One LED, one Button, one Buzzer
// const uint8_t LED_PIN     = 12;     
// const uint8_t BTN_PIN     = 18;     // single button (INPUT_PULLUP)
// const uint8_t BUZZER_PIN  = 23;     // pick any free GPIO (not strapping pins)

// // -------- LEDC PWM (ESP32) --------
// const uint8_t  PWM_LED_CH     = 0;        // 8-bit duty LED
// const uint8_t  PWM_BUZZ_CH    = 3;        // another channel for buzzer
// const uint8_t  PWM_RESOLUTION = 8;
// const uint32_t PWM_LED_FREQ   = 5000;     // LED PWM frequency
// // Buzzer: we'll set tone frequency dynamically with ledcWriteTone()

// // -------- Button press detection --------
// const uint16_t DEBOUNCE_MS     = 30;
// const uint16_t LONG_PRESS_MS   = 1500;

// bool     btnPrev = HIGH;           // because of INPUT_PULLUP
// uint32_t lastEdgeMs = 0;
// uint32_t pressStartMs = 0;

// // -------- LED + buzzer state --------
// bool ledOn = false;

// // -------- OLED message --------
// char     lastEvent[24] = "READY";
// uint32_t messageShownAt = 0;
// const uint16_t MESSAGE_HOLD_MS = 2000;

// // -------- Timers --------
// uint32_t displayRefreshTimer = 0;

// // -------- Helpers --------
// void setLed(bool on) {
//   ledOn = on;
//   ledcWrite(PWM_LED_CH, on ? 255 : 0);
// }

// void beep(uint32_t freqHz, uint16_t durationMs) {
//   // ESP32 Arduino: ledcWriteTone sets channel frequency and starts the tone
//   ledcAttachPin(BUZZER_PIN, PWM_BUZZ_CH);
//   ledcWriteTone(PWM_BUZZ_CH, freqHz);
//   delay(durationMs);
//   ledcWriteTone(PWM_BUZZ_CH, 0);   // stop tone
// }

// void showEvent(const char* msg) {
//   strncpy(lastEvent, msg, sizeof(lastEvent));
//   lastEvent[sizeof(lastEvent) - 1] = '\0';
//   messageShownAt = millis();
// }

// // -------- OLED update --------
// void updateOLED() {
//   display.clearDisplay();
//   display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

//   display.setTextColor(SSD1306_WHITE);

//   // Title
//   display.setTextSize(1);
//   display.setCursor(8, 8);
//   display.print("~ TASK B: BUTTON ~");

//   // Status labels
//   display.setTextSize(2);
//   display.setCursor(8, 26);
//   display.print("LED:");
//   display.setCursor(60, 26);
//   display.print(ledOn ? "ON " : "OFF");

//   display.setTextSize(1);
//   display.setCursor(8, 45);
//   display.print("Event: ");
//   display.print(lastEvent);

//   display.display();
// }

// // -------- Setup --------
// void setup() {
//   Serial.begin(115200);

//   // If using custom I2C pins: Wire.begin(SDA, SCL);
//   if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
//     Serial.println("OLED Init Failed!");
//     while (1);
//   }

//   // LED PWM
//   ledcSetup(PWM_LED_CH, PWM_LED_FREQ, PWM_RESOLUTION);
//   ledcAttachPin(LED_PIN, PWM_LED_CH);
//   setLed(false);

//   // Buzzer channel set up (no tone yet)
//   ledcSetup(PWM_BUZZ_CH, 2000 /*placeholder*/, PWM_RESOLUTION);
//   // Attach when beeping (we attach in beep() too, harmless if repeated)

//   // Button
//   pinMode(BTN_PIN, INPUT_PULLUP);

//   updateOLED();
//   Serial.println("Task B Ready");
// }

// // -------- Loop --------
// void loop() {
//   uint32_t now = millis();

//   // --- Button edge detection with debounce ---
//   bool btn = digitalRead(BTN_PIN);          // HIGH=idle, LOW=pressed
//   if (btn != btnPrev && (now - lastEdgeMs) >= DEBOUNCE_MS) {
//     lastEdgeMs = now;

//     if (btn == LOW) {
//       // Press start
//       pressStartMs = now;
//     } else {
//       // Released -> classify
//       uint32_t held = now - pressStartMs;
//       if (held >= LONG_PRESS_MS) {
//         // Long press: play a buzzer tone
//         showEvent("LONG PRESS");
//         // Play a simple 2-tone chirp
//         beep(1200, 120);
//         beep(1800, 120);
//       } else {
//         // Short press: toggle LED
//         setLed(!ledOn);
//         showEvent("SHORT PRESS");
//       }
//     }

//     btnPrev = btn;
//   }

//   // Auto-clear message after a while (optional)
//   if (messageShownAt && (now - messageShownAt) > MESSAGE_HOLD_MS) {
//     strncpy(lastEvent, "READY", sizeof(lastEvent));
//     lastEvent[sizeof(lastEvent) - 1] = '\0';
//     messageShownAt = 0;
//   }

//   // Refresh OLED (throttled)
//   if (now - displayRefreshTimer >= 100) {
//     displayRefreshTimer = now;
//     updateOLED();
//   }
// }
