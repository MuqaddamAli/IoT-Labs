#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ========== DISPLAY SETUP ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ========== PIN DEFINITIONS ==========
const uint8_t LED_RED    = 12;
const uint8_t LED_YELLOW = 14;
const uint8_t LED_GREEN  = 27;
const uint8_t BTN_CYCLE  = 18;  // Button to cycle through modes
const uint8_t BTN_HOME   = 19;  // Button to return to sleep mode

// ========== PWM CHANNELS ==========
const uint8_t PWM_RED_CH    = 0;
const uint8_t PWM_YELLOW_CH = 1;
const uint8_t PWM_GREEN_CH  = 2;

// ========== MODE SYSTEM ==========
enum DisplayMode {
  MODE_SLEEP = 0,
  MODE_DANCE = 1,
  MODE_PARTY = 2,
  MODE_BREATHE = 3,
  MODE_COUNT = 4  // Total number of modes
};
DisplayMode currentMode = MODE_SLEEP;

// ========== BUTTON FLAGS ==========
volatile bool requestModeChange = false;
volatile bool requestReset = false;
volatile uint32_t lastCyclePress = 0;
volatile uint32_t lastHomePress = 0;

// ========== ANIMATION TIMERS ==========
uint32_t animationTimer = 0;
uint32_t displayTimer = 0;
bool blinkState = false;

// ========== BUTTON INTERRUPT HANDLERS ==========
void IRAM_ATTR handleCycleButton() {
  uint32_t now = millis();
  if (now - lastCyclePress > 250) {  // 250ms debounce
    requestModeChange = true;
    lastCyclePress = now;
  }
}

void IRAM_ATTR handleHomeButton() {
  uint32_t now = millis();
  if (now - lastHomePress > 250) {  // 250ms debounce
    requestReset = true;
    lastHomePress = now;
  }
}

// ========== LED CONTROL FUNCTIONS ==========
void setAllLEDs(uint8_t red, uint8_t yellow, uint8_t green) {
  ledcWrite(PWM_RED_CH, red);
  ledcWrite(PWM_YELLOW_CH, yellow);
  ledcWrite(PWM_GREEN_CH, green);
}

void clearAllLEDs() {
  setAllLEDs(0, 0, 0);
}

uint8_t calculateBreathingValue() {
  // Create smooth breathing effect using cosine wave
  uint32_t t = millis() % 2500;  // 2.5 second cycle
  float phase = (float)t / 2500.0f;
  float wave = 0.5f * (1.0f - cosf(2.0f * PI * phase));  // 0.0 to 1.0
  
  // Apply gamma correction for perceptual smoothness
  float corrected = powf(wave, 2.2f);
  
  // Map to PWM range (3 to 255)
  return (uint8_t)(3 + corrected * 252 + 0.5f);
}

// ========== DISPLAY UPDATE ==========
void updateDisplay() {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setTextSize(1);
  display.setCursor(35, 8);
  display.print("~ MODE ~");

  // Mode name (large text)
  display.setTextSize(2);
  display.setCursor(15, 24);
  
  // Emoticon (small text)
  display.setTextSize(1);
  display.setCursor(85, 28);

  switch(currentMode) {
    case MODE_SLEEP:
      display.setCursor(15, 24);
      display.setTextSize(2);
      display.print("SLEEP");
      display.setTextSize(1);
      display.setCursor(85, 28);
      display.print("Zzz");
      break;

    case MODE_DANCE:
      display.setCursor(15, 24);
      display.setTextSize(2);
      display.print("BLINK");
      display.setTextSize(1);
      display.setCursor(85, 28);
      display.print(blinkState ? "(^_^)" : "(-_-)");
      break;

    case MODE_PARTY:
      display.setCursor(15, 24);
      display.setTextSize(2);
      display.print("PARTY!");
      display.setTextSize(1);
      display.setCursor(85, 28);
      display.print("(*_*)");
      break;

    case MODE_BREATHE:
      display.setCursor(15, 24);
      display.setTextSize(2);
      display.print("CHILL");
      display.setTextSize(1);
      display.setCursor(85, 28);
      display.print("(^_^)");
      break;
  }

  // Footer
  display.setCursor(5, 50);
  display.print("[");
  display.print((int)currentMode);
  display.print("/3] Press to cycle");

  display.display();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Setup PWM channels
  ledcSetup(PWM_RED_CH, 5000, 8);      // 5kHz frequency, 8-bit resolution
  ledcSetup(PWM_YELLOW_CH, 5000, 8);
  ledcSetup(PWM_GREEN_CH, 5000, 8);
  
  // Attach pins to PWM channels
  ledcAttachPin(LED_RED, PWM_RED_CH);
  ledcAttachPin(LED_YELLOW, PWM_YELLOW_CH);
  ledcAttachPin(LED_GREEN, PWM_GREEN_CH);

  // Setup buttons with pull-up resistors
  pinMode(BTN_CYCLE, INPUT_PULLUP);
  pinMode(BTN_HOME, INPUT_PULLUP);
  
  // Attach interrupts (trigger on button press - falling edge)
  attachInterrupt(digitalPinToInterrupt(BTN_CYCLE), handleCycleButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_HOME), handleHomeButton, FALLING);

  clearAllLEDs();
  updateDisplay();
}

// ========== MAIN LOOP ==========
void loop() {
  uint32_t now = millis();

  // Handle button presses
  if (requestReset) {
    currentMode = MODE_SLEEP;
    clearAllLEDs();
    requestReset = false;
  }

  if (requestModeChange) {
    currentMode = (DisplayMode)((currentMode + 1) % MODE_COUNT);
    clearAllLEDs();
    requestModeChange = false;
  }

  // Handle LED animations based on current mode
  switch(currentMode) {
    case MODE_SLEEP:
      // All LEDs off
      clearAllLEDs();
      break;

    case MODE_DANCE:
      // Alternate between two patterns every 400ms
      if (now - animationTimer >= 400) {
        animationTimer = now;
        blinkState = !blinkState;
        
        if (blinkState) {
          setAllLEDs(255, 0, 255);  // 2 outer ones
        } else {
          setAllLEDs(0, 255, 0);    // 1 inner one
        }
      }
      break;

    case MODE_PARTY:
      // All LEDs on full brightness
      setAllLEDs(255, 255, 255);
      break;

    case MODE_BREATHE:
      // Smooth breathing effect on all LEDs
      uint8_t brightness = calculateBreathingValue();
      setAllLEDs(brightness, brightness, brightness);
      break;
  }

  // Update display every 100ms
  if (now - displayTimer >= 100) {
    displayTimer = now;
    updateDisplay();
  }
}


// ---- TASK 2 ----

// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // ========== DISPLAY SETUP ==========
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// // ========== PIN DEFINITIONS ==========
// const uint8_t LED_PIN    = 12;
// const uint8_t BTN_PIN    = 18;
// const uint8_t BUZZER_PIN = 23;

// // ========== PWM CHANNELS ==========
// const uint8_t PWM_LED_CH  = 0;  // Channel for LED brightness control
// const uint8_t PWM_BUZZ_CH = 3;  // Channel for buzzer tones

// // ========== BUTTON TIMING ==========
// const uint16_t DEBOUNCE_MS   = 200;    
// const uint16_t LONG_PRESS_MS = 1500;  // 1.5s for long press

// // ========== BUTTON STATE ==========
// bool lastButtonState = HIGH;  // HIGH = not pressed (pull-up resistor)
// uint32_t lastChangeTime = 0;  // When button state last changed
// uint32_t pressStartTime = 0;  // When button was pressed down

// // ========== LED STATE ==========
// bool ledIsOn = false;

// // ========== DISPLAY MESSAGE ==========
// char eventMessage[24] = "READY";
// uint32_t messageTime = 0;

// // ========== TIMERS ==========
// uint32_t displayTimer = 0;

// // ========== LED CONTROL ==========
// void setLED(bool on) {
//   ledIsOn = on;
//   ledcWrite(PWM_LED_CH, on ? 255 : 0);
// }

// // ========== BUZZER CONTROL ==========
// void playBeep(uint32_t frequency, uint16_t durationMs) {
//   ledcAttachPin(BUZZER_PIN, PWM_BUZZ_CH);
//   ledcWriteTone(PWM_BUZZ_CH, frequency);
//   delay(durationMs);
//   ledcWriteTone(PWM_BUZZ_CH, 0);  // Stop tone
// }

// void playChirp() {
//   playBeep(1200, 120);  // Low tone
//   playBeep(1800, 120);  // High tone
// }

// // ========== MESSAGE DISPLAY ==========
// void showMessage(const char* message) {
//   strncpy(eventMessage, message, sizeof(eventMessage) - 1);
//   eventMessage[sizeof(eventMessage) - 1] = '\0';  // Ensure null termination
//   messageTime = millis();
// }

// // ========== OLED UPDATE ==========
// void updateDisplay() {
//   display.clearDisplay();
//   display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
//   display.setTextColor(SSD1306_WHITE);

//   // Title
//   display.setTextSize(1);
//   display.setCursor(8, 8);
//   display.print("~ TASK B: BUTTON ~");

//   // LED status (large text)
//   display.setTextSize(2);
//   display.setCursor(8, 26);
//   display.print("LED:");
//   display.setCursor(60, 26);
//   display.print(ledIsOn ? "ON " : "OFF");

//   // Event message (small text)
//   display.setTextSize(1);
//   display.setCursor(8, 45);
//   display.print("Event: ");
//   display.print(eventMessage);

//   display.display();
// }

// void setup() {
//   Serial.begin(115200);

//   // Initialize display
//   if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
//     Serial.println("OLED Init Failed!");
//     while (1);  // Halt if display fails
//   }

//   // Setup LED PWM
//   ledcSetup(PWM_LED_CH, 5000, 8);  // 5kHz, 8-bit resolution
//   ledcAttachPin(LED_PIN, PWM_LED_CH);
//   setLED(false);

//   // Setup buzzer PWM
//   ledcSetup(PWM_BUZZ_CH, 2000, 8);  // Placeholder frequency

//   // Setup button with pull-up resistor
//   pinMode(BTN_PIN, INPUT_PULLUP);

//   updateDisplay();
// }

// void loop() {
//   uint32_t now = millis();

//   bool currentButtonState = digitalRead(BTN_PIN);  // LOW = pressed, HIGH = released

//   // Check if button state changed (with debouncing)
//   if (currentButtonState != lastButtonState && (now - lastChangeTime) >= DEBOUNCE_MS) {
//     lastChangeTime = now;

//     if (currentButtonState == LOW) {
//       // Button just pressed down
//       pressStartTime = now;
      
//     } else {
//       // Button just released - determine what kind of press it was
//       uint32_t pressDuration = now - pressStartTime;
      
//       if (pressDuration >= LONG_PRESS_MS) {
//         // Long press detected
//         showMessage("LONG PRESS");
//         playChirp();
        
//       } else {
//         // Short press detected
//         setLED(!ledIsOn);  // Toggle LED
//         showMessage("SHORT PRESS");
//       }
//     }

//     lastButtonState = currentButtonState;
//   }

//   // ========== AUTO-CLEAR MESSAGE ==========
//   // Clear event message after 1.5 seconds
//   if (messageTime > 0 && (now - messageTime) > 1500) {
//     showMessage("READY");
//     messageTime = 0;
//   }
  
//   // ========== DISPLAY REFRESH ==========
//   // Update display every 100ms
//   if (now - displayTimer >= 100) {
//     displayTimer = now;
//     updateDisplay();
//   }
// }