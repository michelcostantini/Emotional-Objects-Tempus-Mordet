// +---------------------------------------------+
// | 089020 - PROGETTO DI INGEGNERIA INFORMATICA |
// |      Emotional Objects: Tempus Mordet       |
// |    Michel Costantini - 10907540 - 237028    |
// +---------------------------------------------+

#include <Servo.h>
#include <math.h>
#include <NewPing.h>
#include <Wire.h>
#include <RTClib.h>

// =====================
//   PIN CONFIGURATION
// =====================

// SERVOS
const uint8_t PIN_SERVO_LUNAR = 2;
const uint8_t PIN_SERVO_TEXT = 3;
const uint8_t PIN_SERVO_HOUR = 4;
const uint8_t PIN_SERVO_MINUTE = 5;
const uint8_t PIN_SERVO_PENDULUM = 6;
const uint8_t PIN_SERVO_ROPE = 13;
const uint8_t PIN_SERVO_LOCK = 9;

// SONAR
const uint8_t PIN_SONAR_TRIG = 22;
const uint8_t PIN_SONAR_ECHO = 23;

// LED
const uint8_t PIN_LED_R = 10;
const uint8_t PIN_LED_G = 11;
const uint8_t PIN_LED_B = 12;

// BUTTONS
const uint8_t PIN_BTN_BLUE = 28;
const uint8_t PIN_BTN_RED = 32;
const uint8_t PIN_BTN_YELLOW = 36;
const uint8_t PIN_BTN_GREEN = 40;

// MICRO-SWITCHES
const uint8_t PIN_SWITCH_MINUTE = 44;
const uint8_t PIN_SWITCH_LOCK = 48;

// HALL EFFECT SENSOR
const uint8_t PIN_HALL_SENSOR_HOUR = 47;

// =========
//   ENUMS
// =========

// States of the clock main FSM
enum ClockMode : uint8_t {
  NORMAL_MODE,
  HAUNTED_MODE,
  EMOTIONAL_MODE
};

// Emotional substates
enum ClockEmotion : uint8_t {
  EMOTION_NONE,
  EMOTION_SAD,
  EMOTION_ANGRY,
  EMOTION_HUNGRY,
  EMOTION_HAPPY
};

// States of the internal FSM of the EMOTION_SAD substate
enum GameState : uint8_t {
  GAME_INIT,
  GAME_AMUSED,
  GAME_GENERATE,
  GAME_SHOW_PLAY,
  GAME_SHOW_OFF,
  GAME_WAIT_INPUT,
  GAME_FEEDBACK_ON,
  GAME_FEEDBACK_OFF,
  GAME_WIN_FLASH,
  GAME_LOSE_FLASH
};

// States of the FSM that controls hand movement
enum HandState : uint8_t {
  HAND_IDLE,
  HAND_HOMING,
  HAND_OFFSETTING,
  HAND_READY
};

// Substates of the HAND_IDLE state
enum TickState : uint8_t {
  TICK_IDLE,
  TICK_PULSING
};

// Helper enum to map the indexes of the Servo array to readable names
enum ServoIndex : uint8_t {
  SRV_LUNAR,
  SRV_TEXT,
  SRV_HOUR,
  SRV_MINUTE,
  SRV_PENDULUM,
  SRV_ROPE,
  SRV_LOCK
};

// Helper enum to map color codes to readable names
enum EyeColor : uint8_t {
  COLOR_OFF,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_PURPLE
};

// Helper enum to map the indexes of the Button array to readable names
enum BtnIndex : uint8_t {
  BTN_BLUE,
  BTN_RED,
  BTN_YELLOW,
  BTN_GREEN
};

// ==========================
//   CONFIGURABLE CONSTANTS
// ==========================

// Hand movement speeds
const uint8_t HAND_STOP = 90;
const uint8_t HAND_TICK_SPEED = 95;
const uint8_t HAND_SLOW_SPEED = 110;
const uint8_t HAND_FAST_SPEED = 150;

// Hand tick movement durations
const unsigned long HAND_TICK_DURATION_MS = 300;       // Individual tick duration
const unsigned long MINUTE_HAND_INTERVAL_MS = 1000UL;  // Interval between ticks of the minute hand, here set to 1 second to increase visibility and oddness
const unsigned long HOUR_HAND_INTERVAL_MS = 60000UL;   // Interval between ticks of the hour hand, here set to 1 minute to increase visibility and oddness

// Hand offsets for expressions
const uint16_t HOUR_OFFSET_MOUSTACHE = 3;
const uint16_t MINUTE_OFFSET_MOUSTACHE = 50;
const uint16_t HOUR_OFFSET_EYEBROWS = 0;
const uint16_t MINUTE_OFFSET_EYEBROWS = 5;

// Lunar servo angles (based on emotion)
const uint8_t LUNAR_ANGLE_NORMAL = 0;   // Duplicated for possible variations
const uint8_t LUNAR_ANGLE_SAD = 0;      // This angle shows a starry night
const uint8_t LUNAR_ANGLE_ANGRY = 90;   // This angle shows a tempest
const uint8_t LUNAR_ANGLE_HAPPY = 180;  // This angle shows a sunny sky

// Text servo angles (based on configuration)
const uint8_t TEXT_ANGLE_NORMAL = 0;  // Duplicated for possible variations
const uint8_t TEXT_ANGLE_POUT = 0;
const uint8_t TEXT_ANGLE_SMILE = 180;

// Lock servo angles (based on configuration)
const uint8_t ROPE_ANGLE_TAUT = 10;
const uint8_t ROPE_ANGLE_LOOSE = 170;

// Rope servo angles (based on configuration)
const uint8_t LOCK_ANGLE_OPEN = 0;
const uint8_t LOCK_ANGLE_CLOSED = 90;

// Pendulum servo speed based on emotion
const float PENDULUM_PERIOD_NORMAL = 2000.0;
const float PENDULUM_PERIOD_HAUNTED = 1000.0;
const float PENDULUM_PERIOD_HAPPY = 1500.0;
const float PENDULUM_PERIOD_SAD = 3000.0;
const float PENDULUM_PERIOD_ANGRY = 500.0;

// Pendulum servo movement settings
const uint8_t PENDULUM_SWING_AMPLITUDE = 20;
const uint8_t PENDULUM_CENTER = 80;  // Adjustable for possible servo imprecisions

// Hungry emotion timers
const unsigned long BUILDUP_DURATION = 10000;   // Duration of the first phase
const unsigned long JUMPSCARE_DURATION = 5000;  // Duration of the second phase

// Happy emotion settings
const unsigned long HAPPY_SNAP_DELAY_MS = 60000;    // Duration of the grace period before the clock starts increasing the probability of turning stubborn angry
const unsigned long SNAP_CHECK_INTERVAL_MS = 1000;  // Interval between possible emotional transitions
const uint8_t SNAP_CHANCE_PER_SECOND = 1;           // Unit increase in probability of turning stubborn angry
const uint8_t SNAP_CHANCE_MAX = 70;                 // Maximum probability of turning stubborn angry to increase randomness

// Sonar settings
const unsigned long SONAR_INTERVAL = 500;         // Frequency of sonar ping requests
const unsigned long PRESENCE_TRIGGER_MS = 10000;  // Continuous time the user must stand before the clock to trigger an emotional response
const unsigned long ABSENCE_RESET_MS = 5000;      // Continuous time the user must be gone before the clock resets back to normal behavior
const uint8_t PROXIMITY_THRESHOLD = 100;
const uint8_t MAX_DISTANCE = 150;  // Maximum distance that the sonar checks during ping requests

// Game settings
const uint8_t GAME_WIN_LEVEL = 5;            // Length of the sequence needed to trigger happy emotion
const uint8_t GAME_LOSS_LIMIT = 3;           // Number of losses needed to trigger stubborn angry emotion
const uint8_t GAME_BLINKS = 8;               // Number of led blinks during game animations
const unsigned long GAME_SHOW_ON_MS = 800;   // Duration that an LED color remains on during sequence playback
const unsigned long GAME_SHOW_OFF_MS = 200;  // Duration that an LED color remains off during sequence playback and blinks
const unsigned long GAME_FEEDBACK_MS = 300;  // Duration that an LED color remains on after a correct user button click
const unsigned long GAME_AMUSE_MS = 150;     // Speed of the amused animation

// Button settings
const unsigned long DEBOUNCE_MS = 50;  // Time required for a physical button signal to stabilize
const uint8_t BTN_COUNT = 4;

// ===================
//   STATE VARIABLES
// ===================

// FSM variables (with initialization)
ClockMode currentMode = NORMAL_MODE;
ClockEmotion currentEmotion = EMOTION_NONE;
ClockEmotion lastEmotion = EMOTION_NONE;
GameState gameStatus = GAME_INIT;
HandState hourHandState = HAND_IDLE;
HandState minuteHandState = HAND_IDLE;
TickState hourTickState = TICK_IDLE;
TickState minuteTickState = TICK_IDLE;

// Timers for state transitions
unsigned long hourTickTimer = 0;
unsigned long minuteTickTimer = 0;
unsigned long lastHourTick = 0;
unsigned long lastMinuteTick = 0;
unsigned long modeTimer = 0;
unsigned long nextHauntedDelay = 0;  // randomized after each event
unsigned long hauntedDuration = 0;   // randomized after each event

// User detection variables
unsigned long lastSonarRead = 0;
unsigned long presenceTimer = 0;
unsigned long absenceTimer = 0;
bool isUserDetected = false;

// Hands movement handling variables
uint16_t hourHandOffset = HOUR_OFFSET_MOUSTACHE;
uint16_t minuteHandOffset = MINUTE_OFFSET_MOUSTACHE;
bool hourIsClearing = false;
bool minuteIsClearing = false;
unsigned long hourHandTimer = 0;
unsigned long minuteHandTimer = 0;
unsigned long completeHourLap = 1200;    // Initialized to a default value, recomputed during every homing phase
unsigned long completeMinuteLap = 1200;  // Initialized to a default value, recomputed during every homing phase
unsigned long hourClearTimer = 0;
unsigned long minuteClearTimer = 0;
unsigned long hourLapStart = 0;
unsigned long minuteLapStart = 0;

// Emotion handling variables
bool isEmotionTriggered = false;
bool isStubbornAngry = false;
unsigned long happyStartTime = 0;
unsigned long lastProbCheckTime = 0;
unsigned long hungryStartTime = 0;

// Game handling variables
uint8_t gameSequence[GAME_WIN_LEVEL];
uint8_t gameCurrentLevel = 0;
uint8_t gameStepIndex = 0;
uint8_t blinkCounter = 0;
uint8_t sadLossCounter = 0;
bool blinkState = false;
unsigned long gameTimer = 0;

// Pendulum handling variable
float currentPendulumPeriod = PENDULUM_PERIOD_NORMAL;

// Button handling variables with initialization
const uint8_t BTN_PINS[BTN_COUNT] = { PIN_BTN_BLUE, PIN_BTN_RED, PIN_BTN_YELLOW, PIN_BTN_GREEN };  // Immutable array of buttons
bool btnState[BTN_COUNT] = { false, false, false, false };                                         // Array of stabilized button reads
bool lastBtnRaw[BTN_COUNT] = { false, false, false, false };                                       // Array of unfiltered button reads
unsigned long lastBtnTime[BTN_COUNT] = { 0, 0, 0, 0 };                                             // Array of button timers to filter input with debounce

// ======================
//   HARDWARE VARIABLES
// ======================

// Custom struct to allow non-blocking movement
struct ConcurrentServo {
  Servo instance;
  const uint8_t pin;
  uint8_t currentAngle;
  uint8_t targetAngle;
  unsigned long lastUpdate;
  uint8_t stepDelay;
  bool isContinuous;
};

// Array of Servos with initialization
ConcurrentServo servos[] = {
  { Servo(), PIN_SERVO_LUNAR, LUNAR_ANGLE_NORMAL, LUNAR_ANGLE_NORMAL, 0, 10, false },
  { Servo(), PIN_SERVO_TEXT, TEXT_ANGLE_NORMAL, TEXT_ANGLE_NORMAL, 0, 10, false },
  { Servo(), PIN_SERVO_HOUR, HAND_STOP, HAND_STOP, 0, 0, true },
  { Servo(), PIN_SERVO_MINUTE, HAND_STOP, HAND_STOP, 0, 0, true },
  { Servo(), PIN_SERVO_PENDULUM, HAND_STOP, HAND_STOP, 0, 20, false },
  { Servo(), PIN_SERVO_ROPE, ROPE_ANGLE_LOOSE, ROPE_ANGLE_LOOSE, 0, 1000, false },
  { Servo(), PIN_SERVO_LOCK, LOCK_ANGLE_CLOSED, LOCK_ANGLE_CLOSED, 0, 5, false }
};

const uint8_t TOTAL_SERVOS = sizeof(servos) / sizeof(servos[0]);  // Helper const calculated at compile time

NewPing sonar(PIN_SONAR_TRIG, PIN_SONAR_ECHO, MAX_DISTANCE);  // Sonar instantiated using the NewPing library

RTC_DS3231 rtc;  // Real-Time Clock (RTC) chip instantiated using the RTC library

// =====================
//   HELPER FUNCTIONS
// =====================

// === LEDs (EYES) ===

// Writes PWM values (0-255) to the RGB LED pins
void setEyesRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

// Maps custom color constants to predefined RGB values
void setEyesColor(uint8_t colorCode) {
  switch (colorCode) {
    case COLOR_RED: setEyesRGB(255, 0, 0); break;
    case COLOR_GREEN: setEyesRGB(0, 255, 0); break;
    case COLOR_BLUE: setEyesRGB(0, 0, 255); break;
    case COLOR_YELLOW: setEyesRGB(255, 255, 0); break;
    case COLOR_PURPLE: setEyesRGB(128, 0, 255); break;
    default: setEyesRGB(0, 0, 0); break;  // COLOR_OFF
  }
}

// Cycles smoothly through the rainbow spectrum based on time and a sine wave
void setEyesRainbow(unsigned long currentTime) {
  float angle = (float)currentTime * 0.0075;                                     // Speed multiplier
  int r = (sin(angle) * 127.5) + 127.5;                                          // 0° Phase
  int g = (sin(angle + 2.0944) * 127.5) + 127.5;                                 // +120° Phase shift
  int b = (sin(angle + 4.1888) * 127.5) + 127.5;                                 // +240° Phase shift
  setEyesRGB(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255));  // Value sanitization
}

// === BUTTONS ===

// Updates button inputs using a non-blocking debounce filter
void updateButtons(unsigned long now) {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    bool raw = (digitalRead(BTN_PINS[i]) == LOW);
    if (raw != lastBtnRaw[i]) {  // Physical state change --> timer reset
      lastBtnTime[i] = now;
      lastBtnRaw[i] = raw;
    }
    if (now - lastBtnTime[i] >= DEBOUNCE_MS) {  // Finalize update after filtering
      btnState[i] = raw;
    }
  }
}

// Returns true if at least one debounced button is currently pressed
bool anyButtonPressed() {
  return btnState[BTN_BLUE] || btnState[BTN_RED] || btnState[BTN_YELLOW] || btnState[BTN_GREEN];
}

// Returns true only if no button is pressed
bool allButtonsReleased() {
  return !btnState[BTN_BLUE] && !btnState[BTN_RED] && !btnState[BTN_YELLOW] && !btnState[BTN_GREEN];
}

// Checks active buttons and returns the respective color code
EyeColor pressedButtonColor() {
  if (btnState[BTN_RED]) return COLOR_RED;
  if (btnState[BTN_GREEN]) return COLOR_GREEN;
  if (btnState[BTN_BLUE]) return COLOR_BLUE;
  if (btnState[BTN_YELLOW]) return COLOR_YELLOW;
  return COLOR_OFF;
}

// Maps the random sequence index to its corresponding color code
char sequenceToColor(uint8_t val) {
  switch (val) {
    case 0: return COLOR_RED;
    case 1: return COLOR_GREEN;
    case 2: return COLOR_BLUE;
    case 3: return COLOR_YELLOW;
    default: return COLOR_OFF;
  }
}

// === GAME ===

// Resets Game handling variables
void resetGame() {
  blinkCounter = 0;
  blinkState = false;
  gameStepIndex = 0;
  gameCurrentLevel = 0;
  sadLossCounter = 0;
  gameStatus = GAME_INIT;
  setEyesColor(COLOR_OFF);
}

// ======================
//   GAME STATE MACHINE
// ======================

void handleSadGame(unsigned long currentTime) {
  switch (gameStatus) {

    // Waits for any button press to start the game
    case GAME_INIT:
      setEyesColor(COLOR_BLUE);
      if (anyButtonPressed()) {
        gameTimer = currentTime;
        blinkCounter = 0;
        blinkState = false;
        gameStatus = GAME_AMUSED;
      }
      break;

    // Flashes eyes green/yellow and adjust mouth to smile
    case GAME_AMUSED:
      if (currentTime - gameTimer >= GAME_AMUSE_MS) {
        servos[SRV_TEXT].targetAngle = TEXT_ANGLE_SMILE;
        gameTimer = currentTime;
        blinkState = !blinkState;
        setEyesColor(blinkState ? COLOR_GREEN : COLOR_YELLOW);
        blinkCounter++;
        if (blinkCounter >= GAME_BLINKS) {
          setEyesColor(COLOR_OFF);
          gameCurrentLevel = 0;
          gameStatus = GAME_GENERATE;
        }
      }
      break;

    // Adds a new random step to the sequence array
    case GAME_GENERATE:
      if (currentTime - gameTimer >= 1200) {
        if (gameCurrentLevel < GAME_WIN_LEVEL) {
          gameSequence[gameCurrentLevel] = random(0, 4);
          gameCurrentLevel++;
        }
        gameStepIndex = 0;
        gameTimer = currentTime;
        gameStatus = GAME_SHOW_PLAY;
      }
      break;

    // Turns on the eyes with the color corresponding to the current sequence index
    case GAME_SHOW_PLAY:
      setEyesColor(sequenceToColor(gameSequence[gameStepIndex]));
      if (currentTime - gameTimer >= GAME_SHOW_ON_MS) {
        setEyesColor(COLOR_OFF);
        gameTimer = currentTime;
        gameStatus = GAME_SHOW_OFF;
      }
      break;

    // Turns off the eyes between sequence steps
    case GAME_SHOW_OFF:
      if (currentTime - gameTimer >= GAME_SHOW_OFF_MS) {
        gameStepIndex++;
        if (gameStepIndex < gameCurrentLevel) {
          gameTimer = currentTime;
          gameStatus = GAME_SHOW_PLAY;
        } else {
          gameStepIndex = 0;
          gameStatus = GAME_WAIT_INPUT;
        }
      }
      break;

    // Listens for button presses and validates matching sequence color
    case GAME_WAIT_INPUT:
      {
        EyeColor pressed = pressedButtonColor();
        if (pressed != COLOR_OFF) {
          setEyesColor(pressed);
          gameTimer = currentTime;
          if (pressed == sequenceToColor(gameSequence[gameStepIndex])) {
            gameStatus = GAME_FEEDBACK_ON;
          } else {
            blinkCounter = 0;
            blinkState = false;
            gameStatus = GAME_LOSE_FLASH;
          }
        }
        break;
      }

    // Keeps the eyes on with the correct color just pressed
    case GAME_FEEDBACK_ON:
      if (currentTime - gameTimer >= GAME_FEEDBACK_MS) {
        setEyesColor(COLOR_OFF);
        gameStatus = GAME_FEEDBACK_OFF;
      }
      break;

    // Evaluates progress once the button is physically released
    case GAME_FEEDBACK_OFF:
      if (allButtonsReleased()) {
        gameStepIndex++;
        if (gameStepIndex == gameCurrentLevel) {
          if (gameCurrentLevel == GAME_WIN_LEVEL) {
            blinkCounter = 0;
            blinkState = false;
            gameTimer = currentTime;
            gameStatus = GAME_WIN_FLASH;
          } else {
            gameTimer = currentTime;
            gameStatus = GAME_GENERATE;
          }
        } else {
          gameStatus = GAME_WAIT_INPUT;
        }
      }
      break;

    // Flashes green eyes and transitions to the happy emotion
    case GAME_WIN_FLASH:
      if (currentTime - gameTimer >= GAME_SHOW_OFF_MS) {
        gameTimer = currentTime;
        blinkState = !blinkState;
        setEyesColor(blinkState ? COLOR_GREEN : COLOR_OFF);
        blinkCounter++;
        if (blinkCounter >= GAME_BLINKS) {
          setEyesColor(COLOR_OFF);
          gameStatus = GAME_INIT;
          currentEmotion = EMOTION_HAPPY;
        }
      }
      break;

    // Flashes red eyes, resets expression and transitions to the stubborn angry emotion if losses exceed the limit
    case GAME_LOSE_FLASH:
      if (currentTime - gameTimer >= GAME_SHOW_OFF_MS) {
        gameTimer = currentTime;
        servos[SRV_TEXT].targetAngle = TEXT_ANGLE_POUT;
        blinkState = !blinkState;
        setEyesColor(blinkState ? COLOR_RED : COLOR_OFF);
        blinkCounter++;
        if (blinkCounter >= GAME_BLINKS) {
          setEyesColor(COLOR_OFF);
          gameStatus = GAME_INIT;
          sadLossCounter++;
          if (sadLossCounter >= GAME_LOSS_LIMIT) {
            currentEmotion = EMOTION_ANGRY;
            isStubbornAngry = true;
            sadLossCounter = 0;
          }
        }
      }
      break;
  }
}


// ==================================
//   HAND POSITIONING STATE MACHINE
// ==================================

void handleHandPositioning(unsigned long currentTime) {
  // Ignore the call if clock is ticking or rotating disks
  if (currentEmotion == EMOTION_HUNGRY) return;
  if (currentMode == NORMAL_MODE || currentMode == HAUNTED_MODE) return;
  if (servos[SRV_LUNAR].currentAngle != servos[SRV_LUNAR].targetAngle || servos[SRV_TEXT].currentAngle != servos[SRV_TEXT].targetAngle) return;

  switch (hourHandState) {
    static bool hourHomeFound = false;

    // Measures 360° rotation duration across two consecutive switch hits
    case HAND_HOMING:
      servos[SRV_HOUR].targetAngle = HAND_SLOW_SPEED;
      // Ignores input while still on switch
      if (hourIsClearing) {
        if (currentTime - hourClearTimer >= 500) {
          hourIsClearing = false;
        }
      } else if (digitalRead(PIN_HALL_SENSOR_HOUR) == LOW) {
        if (!hourHomeFound) {
          // First hit: starts full lap calibration timer
          hourLapStart = currentTime;
          hourHomeFound = true;
          hourClearTimer = currentTime;
          hourIsClearing = true;
        } else {
          // Second hit: stores complete lap duration
          completeHourLap = currentTime - hourLapStart;
          hourHandTimer = currentTime;
          hourHandState = HAND_OFFSETTING;
          hourHomeFound = false;
        }
      }
      break;

    // Moves for a fraction of the full lap time corresponding to target hour (1/12 scale)
    case HAND_OFFSETTING:
      servos[SRV_HOUR].targetAngle = HAND_SLOW_SPEED;
      if (currentTime - hourHandTimer >= hourHandOffset * completeHourLap / 12) {
        servos[SRV_HOUR].targetAngle = HAND_STOP;
        hourHandState = HAND_READY;
      }
      break;

    // Locks hand at stop position unless manually overridden by the yellow button
    case HAND_READY:
      if (!btnState[BTN_YELLOW]) {
        servos[SRV_HOUR].targetAngle = HAND_STOP;
      }
      break;
    default:
      break;
  }

  switch (minuteHandState) {
    static bool minuteHomeFound = false;

    // Homing phase: Measure 360° rotation duration across two consecutive switch hits
    case HAND_HOMING:
      servos[SRV_MINUTE].targetAngle = HAND_SLOW_SPEED;
      // Ignores input while still on switch
      if (minuteIsClearing) {
        if (currentTime - minuteClearTimer >= 500) {
          minuteIsClearing = false;
        }
      } else if (digitalRead(PIN_SWITCH_MINUTE) == LOW) {
        if (!minuteHomeFound) {
          // First hit: starts full lap calibration timer
          minuteLapStart = currentTime;
          minuteHomeFound = true;
          minuteClearTimer = currentTime;
          minuteIsClearing = true;
        } else {
          // Second hit: stores complete lap duration
          completeMinuteLap = currentTime - minuteLapStart;
          minuteHandTimer = currentTime;
          minuteHandState = HAND_OFFSETTING;
          minuteHomeFound = false;
        }
      }
      break;

    // Moves for a fraction of the full lap time corresponding to target minute (1/60 scale)
    case HAND_OFFSETTING:
      servos[SRV_MINUTE].targetAngle = HAND_SLOW_SPEED;
      if (currentTime - minuteHandTimer >= minuteHandOffset * completeMinuteLap / 60) {
        servos[SRV_MINUTE].targetAngle = HAND_STOP;
        minuteHandState = HAND_READY;
      }
      break;

    // Locks hand at stop position unless manually overridden by the yellow button
    case HAND_READY:
      if (!btnState[BTN_YELLOW]) {
        servos[SRV_MINUTE].targetAngle = HAND_STOP;
      }
      break;
    default:
      break;
  }
}

// ======================
//   EMOTION TRANSITION
// ======================

void applyEmotionTransition(unsigned long currentTime) {
  // Starts emotion timers
  if (currentEmotion == EMOTION_HAPPY) {
    happyStartTime = currentTime;
    lastProbCheckTime = currentTime;
  }
  if (currentEmotion == EMOTION_HUNGRY) {
    hungryStartTime = currentTime;
  }

  // Sets expression based on emotion
  switch (currentEmotion) {
    case EMOTION_SAD:
      servos[SRV_LUNAR].targetAngle = LUNAR_ANGLE_SAD;
      servos[SRV_TEXT].targetAngle = TEXT_ANGLE_POUT;
      break;
    case EMOTION_ANGRY:
      servos[SRV_LUNAR].targetAngle = LUNAR_ANGLE_ANGRY;
      servos[SRV_TEXT].targetAngle = TEXT_ANGLE_POUT;
      break;
    case EMOTION_HAPPY:
      servos[SRV_LUNAR].targetAngle = LUNAR_ANGLE_HAPPY;
      servos[SRV_TEXT].targetAngle = TEXT_ANGLE_SMILE;
      break;
    case EMOTION_NONE:
    default:
      servos[SRV_LUNAR].targetAngle = LUNAR_ANGLE_NORMAL;
      servos[SRV_TEXT].targetAngle = TEXT_ANGLE_NORMAL;
      break;
  }

  // Sets hand configuration based on emotion
  if (currentEmotion == EMOTION_ANGRY) {
    hourHandOffset = HOUR_OFFSET_EYEBROWS;
    minuteHandOffset = MINUTE_OFFSET_EYEBROWS;
  } else {
    hourHandOffset = HOUR_OFFSET_MOUSTACHE;
    minuteHandOffset = MINUTE_OFFSET_MOUSTACHE;
  }

  // Starts the ticking or the homing phase to reach target configuration
  if (currentEmotion == EMOTION_NONE || currentEmotion == EMOTION_HUNGRY) {
    hourHandState = HAND_IDLE;
    minuteHandState = HAND_IDLE;
  } else {
    hourHandState = HAND_HOMING;
    minuteHandState = HAND_HOMING;
  }
}


// =========
//   SETUP
// =========

void setup() {
  //Serial.begin(115200);  // To uncomment for debugging

  // Initializes LEDs
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  // Initializes buttons using internal resistance
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }

  // Initializes Hall effect sensor and micro-switches using internal resistance to reduce space
  pinMode(PIN_HALL_SENSOR_HOUR, INPUT_PULLUP);
  pinMode(PIN_SWITCH_MINUTE, INPUT_PULLUP);
  pinMode(PIN_SWITCH_LOCK, INPUT_PULLUP);

  // Initializes servos and waits for positioning
  servos[SRV_PENDULUM].instance.attach(PIN_SERVO_PENDULUM);
  servos[SRV_PENDULUM].instance.write(90);
  servos[SRV_PENDULUM].lastUpdate = millis();

  for (uint8_t i = 0; i < TOTAL_SERVOS; i++) {
    if (servos[i].isContinuous || i == SRV_PENDULUM) {
      continue;
    }
    servos[i].instance.attach(servos[i].pin);
    servos[i].instance.write(servos[i].targetAngle);
    servos[i].currentAngle = servos[i].targetAngle;
    servos[i].lastUpdate = millis();
    delay(1000);
    servos[i].instance.detach();
  }

  // Initialiazes RTC chip
  Wire.begin();
  if (!rtc.begin()) {
    setEyesColor(COLOR_RED);  // Signals an error
    while (1)
      ;  // Halts execution if RTC is missing
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Sets the time of the RTC chip (to uncomment only for first time use of the RTC chip)
  // Initializes timers and randomness
  randomSeed(analogRead(A0));
  nextHauntedDelay = random(30000, 60000);
  modeTimer = millis();
  lastHourTick = millis();
  lastMinuteTick = millis();
}


// ========
//   LOOP
// ========

void loop() {
  // Updates system timestamp and button arrays
  unsigned long currentTime = millis();
  updateButtons(currentTime);
  // User detection handling with data sanitization using Exponential Moving Average (EMA) filter and sonar error allowance
  static uint8_t absenceCount = 0;
  if (currentTime - lastSonarRead >= SONAR_INTERVAL) {
    lastSonarRead = currentTime;
    unsigned int rawDistance = sonar.ping() / US_ROUNDTRIP_CM;
    if (rawDistance > 0) {
      absenceCount = 0;
      static float filteredDistance = rawDistance;
      filteredDistance = (filteredDistance * 0.7) + (rawDistance * 0.3);
      isUserDetected = (filteredDistance < PROXIMITY_THRESHOLD);
    } else {
      absenceCount++;
      if (absenceCount >= 3) {
        isUserDetected = false;
        absenceCount = 0;
      }
    }
  }

  // User permanence handling
  if (isUserDetected) {
    absenceTimer = 0;
    if (presenceTimer == 0) presenceTimer = currentTime;
    // Triggers emotional response if user remains present beyond threshold
    if (currentTime - presenceTimer >= PRESENCE_TRIGGER_MS && !isEmotionTriggered) {
      currentMode = EMOTIONAL_MODE;
      if (digitalRead(PIN_SWITCH_LOCK) == HIGH) {
        currentEmotion = EMOTION_ANGRY;
        isStubbornAngry = false;
      } else {
        currentEmotion = (random(0, 2) == 0) ? EMOTION_SAD : EMOTION_HUNGRY;
      }
      hourTickState = TICK_IDLE;
      minuteTickState = TICK_IDLE;
      servos[SRV_HOUR].targetAngle = HAND_STOP;
      servos[SRV_MINUTE].targetAngle = HAND_STOP;
      isEmotionTriggered = true;
      resetGame();
    }
  } else {
    presenceTimer = 0;
    if (absenceTimer == 0) absenceTimer = currentTime;
    // Resets emotional FSM and restores normal mode if user remains absent
    if (currentTime - absenceTimer >= ABSENCE_RESET_MS) {
      if (currentMode == EMOTIONAL_MODE || isEmotionTriggered) {
        currentMode = NORMAL_MODE;
        currentEmotion = EMOTION_NONE;
        isEmotionTriggered = false;
        isStubbornAngry = false;
        setEyesColor(COLOR_OFF);
        modeTimer = currentTime;
        nextHauntedDelay = random(30000, 60000);
        hourTickState = TICK_IDLE;
        minuteTickState = TICK_IDLE;
        servos[SRV_HOUR].targetAngle = HAND_STOP;
        servos[SRV_MINUTE].targetAngle = HAND_STOP;
        lastHourTick = currentTime;
        lastMinuteTick = currentTime;
      }
      absenceTimer = 0;
    }
  }

  // Detects emotion transition and handles new state
  if (currentEmotion != lastEmotion) {
    applyEmotionTransition(currentTime);
    lastEmotion = currentEmotion;
  }

  // ============
  //   MAIN FSM
  // ============
  if (currentMode == NORMAL_MODE) {
    // Resets servos
    currentPendulumPeriod = PENDULUM_PERIOD_NORMAL;
    servos[SRV_ROPE].targetAngle = ROPE_ANGLE_LOOSE;

    // Non-blocking internal ticking FSM - minute hand
    if (minuteTickState == TICK_IDLE) {
      if (currentTime - lastMinuteTick >= MINUTE_HAND_INTERVAL_MS) {
        lastMinuteTick = currentTime;
        minuteTickTimer = currentTime;
        minuteTickState = TICK_PULSING;
      }
      servos[SRV_MINUTE].targetAngle = HAND_STOP;
    } else if (minuteTickState == TICK_PULSING) {
      servos[SRV_MINUTE].targetAngle = HAND_TICK_SPEED;
      if (currentTime - minuteTickTimer >= HAND_TICK_DURATION_MS) {
        servos[SRV_MINUTE].targetAngle = HAND_STOP;
        minuteTickState = TICK_IDLE;
      }
    }

    // Non-blocking internal ticking FSM - hour hand
    if (hourTickState == TICK_IDLE) {
      if (currentTime - lastHourTick >= HOUR_HAND_INTERVAL_MS) {
        lastHourTick = currentTime;
        hourTickTimer = currentTime;
        hourTickState = TICK_PULSING;
      }
      servos[SRV_HOUR].targetAngle = HAND_STOP;
    } else if (hourTickState == TICK_PULSING) {
      servos[SRV_HOUR].targetAngle = HAND_TICK_SPEED;
      if (currentTime - hourTickTimer >= HAND_TICK_DURATION_MS) {
        servos[SRV_HOUR].targetAngle = HAND_STOP;
        hourTickState = TICK_IDLE;
      }
    }

    // Triggers haunted mode
    if (currentTime - modeTimer >= nextHauntedDelay) {
      currentMode = HAUNTED_MODE;
      modeTimer = currentTime;
      hauntedDuration = random(3000, 10000);
    }

  } else if (currentMode == HAUNTED_MODE) {
    // Sets higher speeds for pendulum and hands
    currentPendulumPeriod = PENDULUM_PERIOD_HAUNTED;
    servos[SRV_HOUR].targetAngle = HAND_FAST_SPEED;
    servos[SRV_MINUTE].targetAngle = HAND_FAST_SPEED;

    // Resets normal mode after randomized duration
    if (currentTime - modeTimer >= hauntedDuration) {
      currentMode = NORMAL_MODE;
      modeTimer = currentTime;
      nextHauntedDelay = random(15000, 30000);
      lastMinuteTick = currentTime;
      lastHourTick = currentTime;
      hourTickState = TICK_IDLE;
      minuteTickState = TICK_IDLE;
    }

  } else if (currentMode == EMOTIONAL_MODE) {
    switch (currentEmotion) {
      case EMOTION_SAD:
        currentPendulumPeriod = PENDULUM_PERIOD_SAD;
        handleSadGame(currentTime);  // Redirect execution to game FSM
        break;

      case EMOTION_HUNGRY:
        {
          unsigned long elapsed = currentTime - hungryStartTime;

          // PHASE 1: BUILDUP
          if (elapsed < BUILDUP_DURATION) {
            float progress = (float)elapsed / (float)BUILDUP_DURATION;

            // Resets timers on first execution
            if (elapsed < 40) {
              lastMinuteTick = currentTime;
              minuteTickState = TICK_IDLE;
            }

            // Calculates ticking acceleration
            unsigned long currentInterval = MINUTE_HAND_INTERVAL_MS - (unsigned long)(progress * 900.0f);
            uint8_t currentTickSpeed = HAND_TICK_SPEED + (uint8_t)(progress * (130.0f - HAND_TICK_SPEED));
            unsigned long currentTickDuration = HAND_TICK_DURATION_MS - (unsigned long)(progress * 250.0f);

            // Mimics normal ticking but with increasing speed
            if (minuteTickState == TICK_IDLE) {
              if (currentTime - lastMinuteTick >= currentInterval) {
                lastMinuteTick = currentTime;
                minuteTickTimer = currentTime;
                minuteTickState = TICK_PULSING;
              }
              servos[SRV_HOUR].targetAngle = HAND_STOP;
              servos[SRV_MINUTE].targetAngle = HAND_STOP;
            } else if (minuteTickState == TICK_PULSING) {
              servos[SRV_MINUTE].targetAngle = currentTickSpeed;
              if (currentTime - minuteTickTimer >= currentTickDuration) {
                minuteTickState = TICK_IDLE;
              }
            }

            // Fades eyes to purple with glitch effect
            uint8_t targetRed = (uint8_t)(progress * 128);
            uint8_t targetBlue = (uint8_t)(progress * 255);
            uint8_t glitch = 0;
            if (random(0, 100) < (progress * 70.0f)) {
              glitch = random(50, 100);
            }
            setEyesRGB((targetRed > glitch) ? targetRed - glitch : 0, 0, (targetBlue > glitch * 2) ? targetBlue - glitch * 2 : 0);

            // PHASE 2: JUMPSCARE
          } else if (elapsed < (BUILDUP_DURATION + JUMPSCARE_DURATION)) {
            // Increases speed and pulls the rope to make the tongue stick out
            setEyesRGB(128 - random(0, 128), 0, 255 - random(0, 255));
            servos[SRV_HOUR].targetAngle = HAND_STOP;
            servos[SRV_MINUTE].targetAngle = HAND_STOP;
            servos[SRV_ROPE].targetAngle = ROPE_ANGLE_TAUT;

            // PHASE 3: RESET AND TRANSITION
          } else {
            setEyesRGB(128 - random(0, 128), 0, 255 - random(0, 255));
            servos[SRV_ROPE].targetAngle = ROPE_ANGLE_LOOSE;
            if (elapsed > (BUILDUP_DURATION + JUMPSCARE_DURATION + 1200)) {
              servos[SRV_ROPE].instance.detach();
              currentEmotion = EMOTION_HAPPY;
            }
          }
          break;
        }

      case EMOTION_ANGRY:
        currentPendulumPeriod = PENDULUM_PERIOD_ANGRY;
        setEyesColor(COLOR_RED);
        // Triggers happy emotion when lock is closed
        if (!isStubbornAngry && digitalRead(PIN_SWITCH_LOCK) == LOW && minuteHandState == HAND_READY && hourHandState == HAND_READY) {
          currentEmotion = EMOTION_HAPPY;
        }
        break;

      case EMOTION_HAPPY:
        {
          currentPendulumPeriod = PENDULUM_PERIOD_NORMAL;
          static bool blueButtonWasPressed = false;

          // BLUE BUTTON: shows current time
          if (btnState[BTN_BLUE]) {
            if (!blueButtonWasPressed) {
              blueButtonWasPressed = true;
              DateTime nowRTC = rtc.now();
              uint8_t hour = nowRTC.hour() % 12;
              hourHandOffset = (hour) > 10 ? hour : (10 - hour);
              minuteHandOffset = 60 - nowRTC.minute() + 5;

              hourHandState = HAND_HOMING;
              minuteHandState = HAND_HOMING;
            }
          } else {
            blueButtonWasPressed = false;
          }

          // RED BUTTON: accelerates pendulum speed
          if (btnState[BTN_RED]) {
            currentPendulumPeriod = PENDULUM_PERIOD_HAUNTED;
          } else {
            currentPendulumPeriod = PENDULUM_PERIOD_NORMAL;
          }

          // YELLOW BUTTON: accelerates hands speed
          if (hourHandState != HAND_HOMING && hourHandState != HAND_OFFSETTING && minuteHandState != HAND_HOMING && minuteHandState != HAND_OFFSETTING) {
            if (btnState[BTN_YELLOW]) {
              hourHandOffset = HOUR_OFFSET_MOUSTACHE;
              minuteHandOffset = MINUTE_OFFSET_MOUSTACHE;
              hourHandState = HAND_READY;
              minuteHandState = HAND_READY;
              servos[SRV_HOUR].targetAngle = HAND_FAST_SPEED;
              servos[SRV_MINUTE].targetAngle = HAND_FAST_SPEED;
            } else if (servos[SRV_HOUR].targetAngle == HAND_FAST_SPEED) {
              hourHandState = HAND_HOMING;
              minuteHandState = HAND_HOMING;
            }
          }

          // GREEN BUTTON: starts eye rainbow cycle
          if (btnState[BTN_GREEN]) {
            setEyesRainbow(currentTime);
          } else {
            setEyesColor(COLOR_GREEN);
          }

          // After the grace period, continuously increases probability to transition to stubborn angry emotion
          unsigned long elapsedHappy = currentTime - happyStartTime;
          if (elapsedHappy >= HAPPY_SNAP_DELAY_MS) {
            if (currentTime - lastProbCheckTime >= SNAP_CHECK_INTERVAL_MS) {
              lastProbCheckTime = currentTime;
              uint32_t secondsPast = (elapsedHappy - HAPPY_SNAP_DELAY_MS) / 1000;
              uint8_t snapChance = (uint8_t)min((uint32_t)(secondsPast * SNAP_CHANCE_PER_SECOND), (uint32_t)SNAP_CHANCE_MAX);
              if (random(0, 100) < snapChance && hourHandState == HAND_READY && minuteHandState == HAND_READY) {
                currentEmotion = EMOTION_ANGRY;
                isStubbornAngry = true;
              }
            }
          }
          break;
        }
    }
  }

  // Invokes hands positioning FSM
  handleHandPositioning(currentTime);

  // Handles lock logic based on lock micro-switch and emotion
  if (currentMode == EMOTIONAL_MODE) {
    if (currentEmotion == EMOTION_SAD || currentEmotion == EMOTION_HAPPY) {
      servos[SRV_LOCK].targetAngle = LOCK_ANGLE_OPEN;
    } else if (currentEmotion == EMOTION_ANGRY) {
      servos[SRV_LOCK].targetAngle = LOCK_ANGLE_CLOSED;
    } else {
      servos[SRV_LOCK].targetAngle = (digitalRead(PIN_SWITCH_LOCK) == LOW) ? LOCK_ANGLE_CLOSED : LOCK_ANGLE_OPEN;
    }
  } else {
    servos[SRV_LOCK].targetAngle = (digitalRead(PIN_SWITCH_LOCK) == LOW) ? LOCK_ANGLE_CLOSED : LOCK_ANGLE_OPEN;
  }

  // Handles realistic pendulum movement using a sine wave
  if (currentTime - servos[SRV_PENDULUM].lastUpdate >= servos[SRV_PENDULUM].stepDelay) {  // Update at 50Hz
    servos[SRV_PENDULUM].lastUpdate = currentTime;
    float pendulumPhase = fmod((float)currentTime / currentPendulumPeriod, 1.0f);
    float rawSin = sin(2.0 * PI * pendulumPhase);
    int pendulumAngle = PENDULUM_CENTER + (int)(PENDULUM_SWING_AMPLITUDE * rawSin);
    servos[SRV_PENDULUM].instance.write((uint8_t)constrain(pendulumAngle, PENDULUM_CENTER - PENDULUM_SWING_AMPLITUDE, PENDULUM_CENTER + PENDULUM_SWING_AMPLITUDE));
  }

  // Handles snapping rope movement
  static bool start = true;
  if (servos[SRV_ROPE].currentAngle != servos[SRV_ROPE].targetAngle) {
    if (start && (!servos[SRV_ROPE].instance.attached() || servos[SRV_ROPE].targetAngle == ROPE_ANGLE_LOOSE)) {
      servos[SRV_ROPE].instance.attach(servos[SRV_ROPE].pin);
      servos[SRV_ROPE].instance.write(servos[SRV_ROPE].targetAngle);
      servos[SRV_ROPE].lastUpdate = currentTime;
      start = false;
    }
    if (currentTime - servos[SRV_ROPE].lastUpdate >= servos[SRV_ROPE].stepDelay) {
      servos[SRV_ROPE].currentAngle = servos[SRV_ROPE].targetAngle;
      start = true;
    }
  } else {
    // Keeps the rope servo attached if taut to preserve needed torque
    if (servos[SRV_ROPE].instance.attached() && servos[SRV_ROPE].targetAngle != ROPE_ANGLE_TAUT) {
      servos[SRV_ROPE].instance.detach();
    }
  }

  // CONCURRENT SERVOS UPDATE
  // Writes small step increments towards target angle and detaches non-moving servos to optimize power usage
  for (uint8_t i = 0; i < TOTAL_SERVOS; i++) {
    if (i == SRV_PENDULUM || i == SRV_ROPE) continue;  // Skips standalone servos

    if (servos[i].isContinuous) {
      if (servos[i].targetAngle == HAND_STOP) {
        if (servos[i].instance.attached()) {
          servos[i].instance.write(HAND_STOP);
          servos[i].currentAngle = HAND_STOP;
          servos[i].instance.detach();  // Prevents creeping when stopped
        }
      } else {
        if (!servos[i].instance.attached()) {
          servos[i].instance.attach(servos[i].pin);
        }
        servos[i].currentAngle = servos[i].targetAngle;
        servos[i].instance.write(servos[i].currentAngle);
      }
    } else {
      if (servos[i].currentAngle != servos[i].targetAngle) {
        if (!servos[i].instance.attached()) {
          servos[i].instance.attach(servos[i].pin);
        }

        if (currentTime - servos[i].lastUpdate >= (servos[i].stepDelay + i)) {  // Staggers servo updates
          servos[i].lastUpdate = currentTime;
          if (servos[i].currentAngle < servos[i].targetAngle) servos[i].currentAngle++;
          else if (servos[i].currentAngle > servos[i].targetAngle) servos[i].currentAngle--;
          servos[i].instance.write(servos[i].currentAngle);
        }
      } else {
        if (servos[i].instance.attached()) {
          servos[i].instance.detach();
        }
      }
    }
  }
}
