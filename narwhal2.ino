#include <Bounce.h>
#include <EEPROM.h>
#include <Keypad.h>

/**
   Start - PIN defines
*/
/** Analog pin # for the joystick X axis */
#define STICK_X 9

/** Analog pin # for the joystick Y axis */
#define STICK_Y 8

#define DPAD_DETECTION_PIN 15

/** Digital pin # for the joystick button */
#define JOYSTICK_1_BUTTON_PIN 0

/** Digital pin # for the thumb button */
#define THUMB_BUTTON_PIN 12

/** DPAD Switch Pins */
#define DPAD_UP_PIN 23
#define DPAD_DOWN_PIN 22
#define DPAD_LEFT_PIN 0
#define DPAD_RIGHT_PIN 11

/** RGB LED Pins */
#define RED_LED_PIN 20
#define GREEN_LED_PIN 17
#define BLUE_LED_PIN 16

/**
   Stop - PIN defines
*/

/** DPAD Directions */
#define DPAD_UP 1
#define DPAD_DOWN 2
#define DPAD_LEFT 3
#define DPAD_RIGHT 4

/**
  Start - Binding defines
*/
#define BUTTON_JOYSTICK_1_KEY KEY_LEFT_SHIFT
#define BUTTON_THUMB_KEY KEY_SPACE
#define KEYBOARD_MODE_STICK_UP_KEY KEY_UP
#define KEYBOARD_MODE_STICK_DOWN_KEY KEY_DOWN
#define KEYBOARD_MODE_STICK_LEFT_KEY KEY_LEFT
#define KEYBOARD_MODE_STICK_RIGHT_KEY KEY_RIGHT
/**
  Stop - Binding defines
*/

/**
  Start - Keyboard mode defines
*/
byte keyboardModeXStartOffset;
byte keyboardModeYStartOffset;
/**
  Stop - Keyboard mode defines
*/

/**
 * Start - Keypad
 */

byte activeProfileNumber = 1;
const byte switchProfile1Key = 140;
const byte switchProfile2Key = 141;
const byte switchProfile3Key = 142;
const byte nextProfileKey = 143;
const byte previousProfileKey = 144;
const byte holdProfile1Key = 145;
const byte holdProfile2Key = 146;
const byte holdProfile3Key = 147;

const byte ROWS = 5;
const byte COLS = 4;

byte keys[ROWS][COLS] = {
  {'a','f','k','p'},
  {'b','g','l','q'},
  {'c','h','m','r'},
  {'d','i','n','s'},
  {'e','j','o','t'}
};

byte activeKeyProfile[ROWS][COLS];
byte activekeyboardModeStickProfile[4];

byte keyProfile1[ROWS][COLS];
byte joystickButtonProfile1;
byte thumbButtonProfile1;
byte keyboardModeStickProfile1[4];
byte dpadUpProfile1;
byte dpadDownProfile1;
byte dpadLeftProfile1;
byte dpadRightProfile1;
bool isKeyboardModeProfile1;

byte keyProfile2[ROWS][COLS];
byte joystickButtonProfile2;
byte thumbButtonProfile2;
byte keyboardModeStickProfile2[4];
byte dpadUpProfile2;
byte dpadDownProfile2;
byte dpadLeftProfile2;
byte dpadRightProfile2;
bool isKeyboardModeProfile2;

byte keyProfile3[ROWS][COLS];
byte joystickButtonProfile3;
byte thumbButtonProfile3;
byte keyboardModeStickProfile3[4];
byte dpadUpProfile3;
byte dpadDownProfile3;
byte dpadLeftProfile3;
byte dpadRightProfile3;
bool isKeyboardModeProfile3;

byte rowPins[ROWS] = {1, 2, 3, 4, 5}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the kpd

Keypad keyPad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/**
 * Stop - Keypad
 */

int keyProfileRGBRed1;
int keyProfileRGBGreen1;
int keyProfileRGBBlue1;

int keyProfileRGBRed2;
int keyProfileRGBGreen2;
int keyProfileRGBBlue2;

int keyProfileRGBRed3;
int keyProfileRGBGreen3;
int keyProfileRGBBlue3;

short Xstick;
short Ystick;
short deadzone;
short edgeAdjust;
short upperBound;
short lowerBound;
short xLow;
short xHigh;
short yLow;
short yHigh;
bool isKeyboardMode;
unsigned long lastKeyboardBlink;
bool lastBlinkState;
char originalHoldProfileKey;
char heldProfileKey;
bool dpadMode;

// up, down, left, right
bool keyboardModeKeyStatus[4] = {false, false, false, false};
Bounce joystickButton1 = Bounce(JOYSTICK_1_BUTTON_PIN, 10);
Bounce thumbButton = Bounce(THUMB_BUTTON_PIN, 10);
Bounce dpadUpButton = Bounce(DPAD_UP_PIN, 10);
Bounce dpadDownButton = Bounce(DPAD_DOWN_PIN, 10);
Bounce dpadLeftButton = Bounce(DPAD_LEFT_PIN, 10);
Bounce dpadRightButton = Bounce(DPAD_RIGHT_PIN, 10);

const byte SERIAL_DATA_MAX_SIZE = 16;
byte serialData[SERIAL_DATA_MAX_SIZE];

void setup() {
  Serial.begin(9600);
  Joystick.useManualSend(true);
  pinMode(13, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(THUMB_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DPAD_DETECTION_PIN, INPUT_PULLUP);

  isKeyboardMode = false;
  lastKeyboardBlink = 0;
  lastBlinkState = true;
  dpadMode = isDpadMode();

  if (dpadMode) {
    pinMode(DPAD_UP_PIN, INPUT_PULLUP);
    pinMode(DPAD_DOWN_PIN, INPUT_PULLUP);
    pinMode(DPAD_LEFT_PIN, INPUT_PULLUP);
    pinMode(DPAD_RIGHT_PIN, INPUT_PULLUP);
  } else {
    pinMode(JOYSTICK_1_BUTTON_PIN, INPUT_PULLUP);
    setDeadzone();
    setBounds();
    detectStartupFlags();
  }

  Xstick = 512;
  Ystick = 512;

  setProfile1FromEEPROM();
  setProfile2FromEEPROM();
  setProfile3FromEEPROM();
  originalHoldProfileKey = switchProfile1Key;
  switchProfile(switchProfile1Key);
}

void setLedState(int state) {
  digitalWrite(13, state);
}

void doStickCalculations(bool constrainDeadzone = false) {
  Xstick = 1023 - analogRead(STICK_X);
  Ystick = analogRead(STICK_Y);
  
  if (constrainDeadzone) {
    if (isInsideDeadzone(Xstick)) {
      Xstick = 512;
    } else {
      if (Xstick > 512) {
        Xstick = constrain(map((Xstick - deadzone), 513, (xHigh - deadzone), 513, 1023), 513, 1023);
      } else if (Xstick < 512) {
        Xstick = constrain(map((Xstick + deadzone), 513, (xLow + deadzone), 511, 0), 0, 511);
      }
    }

    if (isInsideDeadzone(Ystick)) {
      Ystick = 512;
    } else {
      if (Ystick > 512) {
        Ystick = constrain(map((Ystick - deadzone), 513, (yHigh - deadzone), 513, 1023), 513, 1023);
      } else if (Ystick < 512) {
        Ystick = constrain(map((Ystick + deadzone), 513, (yLow + deadzone), 511, 0), 0, 511);
      }
    }
  }
}

void loop() {
  receiveSerialData();
  
  if (dpadMode) {
    dpadUpButton.update();
    dpadDownButton.update();
    dpadLeftButton.update();
    dpadRightButton.update();
  } else {
    handleKeyboardModeBlink();
    joystickButton1.update();
    doStickCalculations(true);
  }
  
  thumbButton.update();

  if (isKeyboardMode) {
    // up, down, left, right
    bool keyboardModeKeyPress[4] = {false, false, false, false};

    if (Xstick > (512 + keyboardModeXStartOffset)) {
      keyboardModeKeyPress[3] = true;
    } else if (Xstick < (512 - keyboardModeXStartOffset)) {
      keyboardModeKeyPress[2] = true;
    }

    if (Ystick < (512 - keyboardModeYStartOffset)) {
      keyboardModeKeyPress[0] = true;
    } else if (Ystick > (512 + keyboardModeYStartOffset)) {
      keyboardModeKeyPress[1] = true;
    }

    handleKeyboundModeKey(KEYBOARD_MODE_STICK_UP_KEY, keyboardModeKeyPress[0]);
    handleKeyboundModeKey(KEYBOARD_MODE_STICK_DOWN_KEY, keyboardModeKeyPress[1]);
    handleKeyboundModeKey(KEYBOARD_MODE_STICK_LEFT_KEY, keyboardModeKeyPress[2]);
    handleKeyboundModeKey(KEYBOARD_MODE_STICK_RIGHT_KEY, keyboardModeKeyPress[3]);

    if (joystickButton1.fallingEdge()) {
      keyboardPress(getCurrentProfileJoystickKey());
    }

    if (joystickButton1.risingEdge()) {
      keyboardRelease(getCurrentProfileJoystickKey());
    }
  } else {
    if (dpadMode) {
      if (dpadUpButton.fallingEdge()) {
        keyboardPress(getCurrentProfileDpadKey(DPAD_UP));
      }
  
      if (dpadUpButton.risingEdge()) {
        keyboardRelease(getCurrentProfileDpadKey(DPAD_UP));
      }

      if (dpadDownButton.fallingEdge()) {
        keyboardPress(getCurrentProfileDpadKey(DPAD_DOWN));
      }
  
      if (dpadDownButton.risingEdge()) {
        keyboardRelease(getCurrentProfileDpadKey(DPAD_DOWN));
      }

      if (dpadLeftButton.fallingEdge()) {
        keyboardPress(getCurrentProfileDpadKey(DPAD_LEFT));
      }
  
      if (dpadLeftButton.risingEdge()) {
        keyboardRelease(getCurrentProfileDpadKey(DPAD_LEFT));
      }

      if (dpadRightButton.fallingEdge()) {
        keyboardPress(getCurrentProfileDpadKey(DPAD_RIGHT));
      }
  
      if (dpadRightButton.risingEdge()) {
        keyboardRelease(getCurrentProfileDpadKey(DPAD_RIGHT));
      }
    } else {
      Joystick.X(Xstick);
      Joystick.Y(Ystick);
  
      if (joystickButton1.fallingEdge()) {
        Joystick.button(1, 1);
      }
  
      if (joystickButton1.risingEdge()) {
        Joystick.button(1, 0);
      }
  
      Joystick.Z(512);
      Joystick.Zrotate(512);
      Joystick.sliderLeft(0);
      Joystick.sliderRight(0);
      Joystick.hat(-1);
      Joystick.send_now();
    }
  }

  if (thumbButton.fallingEdge()) {
    keyboardPress(getCurrentProfileThumbKey());
  }

  if (thumbButton.risingEdge()) {
    keyboardRelease(getCurrentProfileThumbKey());
  }

  if (keyPad.getKeys()) {
    for (int keyCount = 0; keyCount < LIST_MAX; keyCount++) {
      if (keyPad.key[keyCount].stateChanged) {
        char rawChar = keyPad.key[keyCount].kchar;
        char pressedChar = getCurrentProfileKey(rawChar);
        
        switch (keyPad.key[keyCount].kstate) {
          case PRESSED:
            if (pressedChar == switchProfile1Key ||
                pressedChar == switchProfile2Key ||
                pressedChar == switchProfile3Key
            ) {
              originalHoldProfileKey = pressedChar;
              switchProfile(pressedChar);
            }

            switch (pressedChar) {
              case holdProfile1Key:
                heldProfileKey = rawChar;
                switchProfile(switchProfile1Key);
                break;
              case holdProfile2Key:
                heldProfileKey = rawChar;
                switchProfile(switchProfile2Key);
                break;
              case holdProfile3Key:
                heldProfileKey = rawChar;
                switchProfile(switchProfile3Key);
                break;
              case nextProfileKey:
              case previousProfileKey:
                switchProfile(pressedChar);
                break;
            }
            
            keyboardPress(pressedChar);
            break;
          case RELEASED:
            if (rawChar == heldProfileKey) {
              heldProfileKey = 0;
              switchProfile(originalHoldProfileKey);
            }
            
            keyboardRelease(pressedChar);
            break;
        }
      }
    }
  }
}

void handleKeyboardModeBlink()
{
  if (isKeyboardMode) {
    unsigned long now = millis();
    
    if ((now - lastKeyboardBlink) > 2000) {
      if (lastBlinkState) {
        setRGBLed(0, 0, 0);
      } else {
        setRGBColorToActiveProfile();
      }
      
      lastBlinkState = !lastBlinkState;
      lastKeyboardBlink = now;
    }
  }
}

void setRGBColorToActiveProfile() {
  switch (activeProfileNumber) {
      case 1:
        setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
        break;
      case 2:
        setRGBLed(keyProfileRGBRed2, keyProfileRGBGreen2, keyProfileRGBBlue2);
        break;
      case 3:
        setRGBLed(keyProfileRGBRed3, keyProfileRGBGreen3, keyProfileRGBBlue3);
        break;
    }
}

void keyboardPress(byte key)
{
  switch (key) {
    case 179:
      Keyboard.press(KEY_TAB);
      break;
    case 193:
      Keyboard.press(KEY_CAPS_LOCK);
      break;
    case 129:
      Keyboard.press(MODIFIERKEY_SHIFT);
      break;
    case 133:
      Keyboard.press(MODIFIERKEY_RIGHT_SHIFT);
      break;
    case 128:
      Keyboard.press(MODIFIERKEY_CTRL);
      break;
    case 132:
      Keyboard.press(MODIFIERKEY_RIGHT_CTRL);
      break;
    case 130:
      Keyboard.press(MODIFIERKEY_ALT);
      break;
    case 134:
      Keyboard.press(MODIFIERKEY_RIGHT_ALT);
      break;
    case 176:
      Keyboard.press(KEY_RETURN);
      break;
    case 177:
      Keyboard.press(KEY_ESC);
      break;
    case 178:
      Keyboard.press(KEY_BACKSPACE);
      break;
    case 194:
      Keyboard.press(KEY_F1);
      break;
    case 195:
      Keyboard.press(KEY_F2);
      break;
    case 196:
      Keyboard.press(KEY_F3);
      break;
    case 197:
      Keyboard.press(KEY_F4);
      break;
    case 198:
      Keyboard.press(KEY_F5);
      break;
    case 199:
      Keyboard.press(KEY_F6);
      break;
    case 200:
      Keyboard.press(KEY_F7);
      break;
    case 201:
      Keyboard.press(KEY_F8);
      break;
    case 202:
      Keyboard.press(KEY_F9);
      break;
    case 203:
      Keyboard.press(KEY_F10);
      break;
    case 204:
      Keyboard.press(KEY_F11);
      break;
    case 205:
      Keyboard.press(KEY_F12);
      break;
    case 209:
      Keyboard.press(KEY_INSERT);
      break;
    case 210:
      Keyboard.press(KEY_HOME);
      break;
    case 211:
      Keyboard.press(KEY_PAGE_UP);
      break;
    case 212:
      Keyboard.press(KEY_DELETE);
      break;
    case 213:
      Keyboard.press(KEY_END);
      break;
    case 214:
      Keyboard.press(KEY_PAGE_DOWN);
      break;
    case 218:
      Keyboard.press(KEY_UP);
      break;
    case 217:
      Keyboard.press(KEY_DOWN);
      break;
    case 216:
      Keyboard.press(KEY_LEFT);
      break;
    case 215:
      Keyboard.press(KEY_RIGHT);
      break;
    default:
      Keyboard.press(key);
  }
}

void keyboardRelease(byte key)
{
  switch (key) {
    case 179:
      Keyboard.release(KEY_TAB);
      break;
    case 193:
      Keyboard.release(KEY_CAPS_LOCK);
      break;
    case 129:
      Keyboard.release(MODIFIERKEY_SHIFT);
      break;
    case 133:
      Keyboard.release(MODIFIERKEY_RIGHT_SHIFT);
      break;
    case 128:
      Keyboard.release(MODIFIERKEY_CTRL);
      break;
    case 132:
      Keyboard.release(MODIFIERKEY_RIGHT_CTRL);
      break;
    case 130:
      Keyboard.release(MODIFIERKEY_ALT);
      break;
    case 134:
      Keyboard.release(MODIFIERKEY_RIGHT_ALT);
      break;
    case 176:
      Keyboard.release(KEY_RETURN);
      break;
    case 177:
      Keyboard.release(KEY_ESC);
      break;
    case 178:
      Keyboard.release(KEY_BACKSPACE);
      break;
    case 194:
      Keyboard.release(KEY_F1);
      break;
    case 195:
      Keyboard.release(KEY_F2);
      break;
    case 196:
      Keyboard.release(KEY_F3);
      break;
    case 197:
      Keyboard.release(KEY_F4);
      break;
    case 198:
      Keyboard.release(KEY_F5);
      break;
    case 199:
      Keyboard.release(KEY_F6);
      break;
    case 200:
      Keyboard.release(KEY_F7);
      break;
    case 201:
      Keyboard.release(KEY_F8);
      break;
    case 202:
      Keyboard.release(KEY_F9);
      break;
    case 203:
      Keyboard.release(KEY_F10);
      break;
    case 204:
      Keyboard.release(KEY_F11);
      break;
    case 205:
      Keyboard.release(KEY_F12);
      break;
    case 209:
      Keyboard.release(KEY_INSERT);
      break;
    case 210:
      Keyboard.release(KEY_HOME);
      break;
    case 211:
      Keyboard.release(KEY_PAGE_UP);
      break;
    case 212:
      Keyboard.release(KEY_DELETE);
      break;
    case 213:
      Keyboard.release(KEY_END);
      break;
    case 214:
      Keyboard.release(KEY_PAGE_DOWN);
      break;
    case 218:
      Keyboard.release(KEY_UP);
      break;
    case 217:
      Keyboard.release(KEY_DOWN);
      break;
    case 216:
      Keyboard.release(KEY_LEFT);
      break;
    case 215:
      Keyboard.release(KEY_RIGHT);
      break;
    default:
      Keyboard.release(key);
  }
}

char getCurrentProfileKey(byte pressed)
{
  char key = 0;
  
  switch (pressed) {
    case 'a':
      key = activeKeyProfile[0][0];
      break;
    case 'b':
      key = activeKeyProfile[1][0];
      break;
    case 'c':
      key = activeKeyProfile[2][0];
      break;
    case 'd':
      key = activeKeyProfile[3][0];
      break;
    case 'e':
      key = activeKeyProfile[4][0];
      break;
    case 'f':
      key = activeKeyProfile[0][1];
      break;
    case 'g':
      key = activeKeyProfile[1][1];
      break;
    case 'h':
      key = activeKeyProfile[2][1];
      break;
    case 'i':
      key = activeKeyProfile[3][1];
      break;
    case 'j':
      key = activeKeyProfile[4][1];
      break;
    case 'k':
      key = activeKeyProfile[0][2];
      break;
    case 'l':
      key = activeKeyProfile[1][2];
      break;
    case 'm':
      key = activeKeyProfile[2][2];
      break;
    case 'n':
      key = activeKeyProfile[3][2];
      break;
    case 'o':
      key = activeKeyProfile[4][2];
      break;
    case 'p':
      key = activeKeyProfile[0][3];
      break;
    case 'q':
      key = activeKeyProfile[1][3];
      break;
    case 'r':
      key = activeKeyProfile[2][3];
      break;
    case 's':
      key = activeKeyProfile[3][3];
      break;
    case 't':
      key = activeKeyProfile[4][3];
      break;
  }

  return key;
}

byte getCurrentProfileThumbKey()
{
  byte key = 0;

  switch (activeProfileNumber) {
    case 1:
      key = thumbButtonProfile1;
      break;
    case 2:
      key = thumbButtonProfile2;
      break;
    case 3:
      key = thumbButtonProfile3;
      break;
  }

  return key;
}

byte getCurrentProfileJoystickKey()
{
  byte key = 0;

  switch (activeProfileNumber) {
    case 1:
      key = joystickButtonProfile1;
      break;
    case 2:
      key = joystickButtonProfile2;
      break;
    case 3:
      key = joystickButtonProfile3;
      break;
  }

  return key;
}

byte getCurrentProfileKeyboardModeKey(int pressed)
{
  byte key = 0;

  switch (pressed) {
    case KEYBOARD_MODE_STICK_UP_KEY:
      key = activekeyboardModeStickProfile[0];
      break;
    case KEYBOARD_MODE_STICK_DOWN_KEY:
      key = activekeyboardModeStickProfile[1];
      break;
    case KEYBOARD_MODE_STICK_LEFT_KEY:
      key = activekeyboardModeStickProfile[2];
      break;
    case KEYBOARD_MODE_STICK_RIGHT_KEY:
      key = activekeyboardModeStickProfile[3];
      break;
  }
  
  return key;
}

byte getCurrentProfileDpadKey(int dpadDirection)
{
  byte key = 0;

  switch (activeProfileNumber) {
    case 1:
      switch (dpadDirection) {
        case DPAD_UP:
          key = dpadUpProfile1;
          break;
        case DPAD_DOWN:
          key = dpadDownProfile1;
          break;
        case DPAD_LEFT:
          key = dpadLeftProfile1;
          break;
        case DPAD_RIGHT:
          key = dpadRightProfile1;
          break;
      }
      break;
    case 2:
      switch (dpadDirection) {
        case DPAD_UP:
          key = dpadUpProfile2;
          break;
        case DPAD_DOWN:
          key = dpadDownProfile2;
          break;
        case DPAD_LEFT:
          key = dpadLeftProfile2;
          break;
        case DPAD_RIGHT:
          key = dpadRightProfile2;
          break;
      }
      break;
    case 3:
      switch (dpadDirection) {
        case DPAD_UP:
          key = dpadUpProfile3;
          break;
        case DPAD_DOWN:
          key = dpadDownProfile3;
          break;
        case DPAD_LEFT:
          key = dpadLeftProfile3;
          break;
        case DPAD_RIGHT:
          key = dpadRightProfile3;
          break;
      }
      break;
  }

  return key;
}

void switchProfile(byte profileKey)
{
  Keyboard.releaseAll();
  
  switch (profileKey) {
    case switchProfile1Key:
      memcpy(activeKeyProfile, keyProfile1, sizeof(keyProfile1));
      memcpy(activekeyboardModeStickProfile, keyboardModeStickProfile1, sizeof(keyboardModeStickProfile1));
      setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
      isKeyboardMode = isKeyboardModeProfile1;
      activeProfileNumber = 1;
      writeCurrentActiveProfile();
      break;
    case switchProfile2Key:
      memcpy(activeKeyProfile, keyProfile2, sizeof(keyProfile2));
      memcpy(activekeyboardModeStickProfile, keyboardModeStickProfile2, sizeof(keyboardModeStickProfile2));
      setRGBLed(keyProfileRGBRed2, keyProfileRGBGreen2, keyProfileRGBBlue2);
      isKeyboardMode = isKeyboardModeProfile2;
      activeProfileNumber = 2;
      writeCurrentActiveProfile();
      break;
    case switchProfile3Key:
      memcpy(activeKeyProfile, keyProfile3, sizeof(keyProfile3));
      memcpy(activekeyboardModeStickProfile, keyboardModeStickProfile3, sizeof(keyboardModeStickProfile3));
      setRGBLed(keyProfileRGBRed3, keyProfileRGBGreen3, keyProfileRGBBlue3);
      isKeyboardMode = isKeyboardModeProfile3;
      activeProfileNumber = 3;
      writeCurrentActiveProfile();
      break;
    case nextProfileKey:
      switch (activeProfileNumber) {
        case 1:
          switchProfile(switchProfile2Key);
          break;
        case 2:
          switchProfile(switchProfile3Key);
          break;
        case 3:
          switchProfile(switchProfile1Key);
          break;
      }
      
      break;
    case previousProfileKey:
      switch (activeProfileNumber) {
        case 1:
          switchProfile(switchProfile3Key);
          break;
        case 2:
          switchProfile(switchProfile1Key);
          break;
        case 3:
          switchProfile(switchProfile2Key);
          break;
      }
      
      break;
  }
}

void setRGBLed(int red, int green, int blue)
{
  analogWrite(RED_LED_PIN, constrain(red, 0, 255));
  //analogWrite(GREEN_LED_PIN, constrain(green - 90, 0, 255));
  //analogWrite(BLUE_LED_PIN, constrain(blue - 90, 0, 255));
  analogWrite(GREEN_LED_PIN, constrain(map(green, 0, 255, 0, 45), 0, 255));
  analogWrite(BLUE_LED_PIN, constrain(map(blue, 0, 255, 0, 45), 0, 255));
}

void handleKeyboundModeKey(int key, bool isPress) {
  int keyIndex = -1;

  switch (key) {
    case KEYBOARD_MODE_STICK_UP_KEY:
      keyIndex = 0;
      break;
    case KEYBOARD_MODE_STICK_DOWN_KEY:
      keyIndex = 1;
      break;
    case KEYBOARD_MODE_STICK_LEFT_KEY:
      keyIndex = 2;
      break;
    case KEYBOARD_MODE_STICK_RIGHT_KEY:
      keyIndex = 3;
      break;
  }

  if (keyIndex > -1) {
    if (isPress) {
      if (keyboardModeKeyStatus[keyIndex] == false) {
        keyboardPress(getCurrentProfileKeyboardModeKey(key));
        keyboardModeKeyStatus[keyIndex] = true;
      }
    } else {
      if (keyboardModeKeyStatus[keyIndex] == true) {
        keyboardRelease(getCurrentProfileKeyboardModeKey(key));
        keyboardModeKeyStatus[keyIndex] = false;
      }
    }
  }
}

bool isInsideDeadzone(int rawStickValue) {
  bool returnValue = false;

  if ((rawStickValue > 512 && rawStickValue <= upperBound) ||
      (rawStickValue < 512 && rawStickValue >= lowerBound)
  ) {
    returnValue = true;
  }

  return returnValue;
}

int getDeadzoneAdjustedValue(int value) {
  if (value > 512) {
    value = value - deadzone;
  }

  if (value < 512) {
    value = value + deadzone;
  }

  return value;
}

bool isDpadMode() {
  bool result = true;

  // PIN is pullup, so if it's high it's not connected thus no DPAD and an analog stick instead.
  if (digitalRead(DPAD_DETECTION_PIN)) {
    result = false;
  }

  return result;
}

void setDeadzone() {
  setLedState(HIGH);
  setRGBLed(0, 255, 0);
  deadzone = 0;

  unsigned long startTime = millis();
  unsigned long highValue = 0;

  while ((millis() - startTime) < 3000) {
    doStickCalculations();

    int localXstick = abs(Xstick);
    int localYstick = abs(Ystick);
    int diffX = 0;
    int diffY = 0;

    if (localXstick < 512) {
      diffX = 512 - localXstick;
    }

    if (localXstick > 512) {
      diffX = localXstick - 512;
    }

    if (localYstick < 512) {
      diffY = 512 - localYstick;
    }

    if (localYstick > 512) {
      diffY = localYstick - 512;
    }

    if (diffX >= diffY) {
      highValue = diffX;
    } else {
      highValue = diffY;
    }
  }

  deadzone = highValue;
}

void setBounds() {
  deadzone = deadzone + 7;
  edgeAdjust = deadzone + 25;
  upperBound = 512 + deadzone;
  lowerBound = 512 - deadzone;
  xLow = getLowestXFromEEPROM();
  xHigh = getHighestXFromEEPROM();
  yLow = getLowestYFromEEPROM();
  yHigh = getHighestYFromEEPROM();
  keyboardModeXStartOffset = getKeyboardModeOffsetXFromEEPROM();
  keyboardModeYStartOffset = getKeyboardModeOffsetYFromEEPROM();

  if (xLow == -1) {
    xLow = 325;
  }

  if (xHigh == -1) {
    xHigh = 675;
  }

  if (yLow == -1) {
    yLow = 325;
  }

  if (yHigh == -1) {
    yHigh = 675;
  }
 
  setLedState(LOW);
  setRGBLed(0, 0, 255);
}

void detectStartupFlags() {
  unsigned long startTime = millis();
  unsigned long lastBlink = 0;
  int ledState = LOW;

  while ((millis() - startTime) < 4000) {
    unsigned long now = millis();

    joystickButton1.update();

    if (now - lastBlink >= 500) {
      lastBlink = now;

      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      setLedState(ledState);
    }
  }
  
  setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
}

void initEEPROM()
{
  updateBoundsToEEPROM(325, 675, 325, 675);

  keyboardModeXStartOffset = 250;
  keyboardModeYStartOffset = 250;
  updateKeyboardModeOffsetsToEEPROM();

  keyProfile1[0][0] = '1';
  keyProfile1[1][0] = '2';
  keyProfile1[2][0] = '3';
  keyProfile1[3][0] = '4';
  keyProfile1[4][0] = '5';
  keyProfile1[0][1] = 'q';
  keyProfile1[1][1] = 'w';
  keyProfile1[2][1] = 'e';
  keyProfile1[3][1] = 'r';
  keyProfile1[4][1] = 'y';
  keyProfile1[0][2] = '6';
  keyProfile1[1][2] = '7';
  keyProfile1[2][2] = '8';
  keyProfile1[3][2] = 'd';
  keyProfile1[4][2] = 'f';
  keyProfile1[0][3] = switchProfile2Key; // 129 = LEFT SHIFT
  keyProfile1[1][3] = 'z';
  keyProfile1[2][3] = 'x';
  keyProfile1[3][3] = 'c';
  keyProfile1[4][3] = 'v';
  joystickButtonProfile1 = 130; // 128 = LEFT CTRL, 129 = LEFT ALT
  thumbButtonProfile1 = 32; // 32 = SPACE
  keyboardModeStickProfile1[0] = 218; // 218 = UP ARROW
  keyboardModeStickProfile1[1] = 217; // 217 = DOWN ARROW
  keyboardModeStickProfile1[2] = 216; // 216 = LEFT ARROW
  keyboardModeStickProfile1[3] = 215; // 215 = RIGHT ARROW
  dpadUpProfile1 = 'w';
  dpadDownProfile1 = 's';
  dpadLeftProfile1 = 'a';
  dpadRightProfile1 = 'd';
  //keyProfileRGBRed1 = 255;
  //keyProfileRGBGreen1 = 102;
  //keyProfileRGBBlue1 = 102;
  keyProfileRGBRed1 = 255;
  keyProfileRGBGreen1 = 0;
  keyProfileRGBBlue1 = 0;
  isKeyboardModeProfile1 = false;
  updateProfile1ToEEPROM();

  keyProfile2[0][0] = 't';
  keyProfile2[1][0] = '1';
  keyProfile2[2][0] = '2';
  keyProfile2[3][0] = '3';
  keyProfile2[4][0] = '4';
  keyProfile2[0][1] = 179;
  keyProfile2[1][1] = 'q';
  keyProfile2[2][1] = 'w';
  keyProfile2[3][1] = 'e';
  keyProfile2[4][1] = 'r';
  keyProfile2[0][2] = 129;
  keyProfile2[1][2] = 'a';
  keyProfile2[2][2] = 's';
  keyProfile2[3][2] = 'd';
  keyProfile2[4][2] = 'f';
  keyProfile2[0][3] = switchProfile3Key;
  keyProfile2[1][3] = 'z';
  keyProfile2[2][3] = 'x';
  keyProfile2[3][3] = 'c';
  keyProfile2[4][3] = 'v';
  joystickButtonProfile2 = 128; // 128 = LEFT CTRL
  thumbButtonProfile2 = 32; // 32 = SPACE
  keyboardModeStickProfile2[0] = 218; // 218 = UP ARROW
  keyboardModeStickProfile2[1] = 217; // 217 = DOWN ARROW
  keyboardModeStickProfile2[2] = 216; // 216 = LEFT ARROW
  keyboardModeStickProfile2[3] = 215; // 215 = RIGHT ARROW
  dpadUpProfile2 = 'w';
  dpadDownProfile2 = 's';
  dpadLeftProfile2 = 'a';
  dpadRightProfile2 = 'd';
  keyProfileRGBRed2 = 0;
  keyProfileRGBGreen2 = 255;
  keyProfileRGBBlue2 = 0;
  isKeyboardModeProfile2 = false;
  updateProfile2ToEEPROM();

  keyProfile3[0][0] = '`';
  keyProfile3[1][0] = '1';
  keyProfile3[2][0] = '2';
  keyProfile3[3][0] = '3';
  keyProfile3[4][0] = '4';
  keyProfile3[0][1] = 179; // 179 = TAB
  keyProfile3[1][1] = 'q';
  keyProfile3[2][1] = 'w';
  keyProfile3[3][1] = 'e';
  keyProfile3[4][1] = 'r';
  keyProfile3[0][2] = 129; // 193 = CAPS LOCK
  keyProfile3[1][2] = 'a';
  keyProfile3[2][2] = 's';
  keyProfile3[3][2] = 'd';
  keyProfile3[4][2] = 'f';
  keyProfile3[0][3] = switchProfile1Key; // 129 = LEFT SHIFT
  keyProfile3[1][3] = 'z';
  keyProfile3[2][3] = 'x';
  keyProfile3[3][3] = 'c';
  keyProfile3[4][3] = 'v';
  joystickButtonProfile3 = 128; // 128 = LEFT CTRL
  thumbButtonProfile3 = 32; // 32 = SPACE
  keyboardModeStickProfile3[0] = 218; // 218 = UP ARROW
  keyboardModeStickProfile3[1] = 217; // 217 = DOWN ARROW
  keyboardModeStickProfile3[2] = 216; // 216 = LEFT ARROW
  keyboardModeStickProfile3[3] = 215; // 215 = RIGHT ARROW
  dpadUpProfile3 = 'w';
  dpadDownProfile3 = 's';
  dpadLeftProfile3 = 'a';
  dpadRightProfile3 = 'd';
  keyProfileRGBRed3 = 0;
  keyProfileRGBGreen3 = 0;
  keyProfileRGBBlue3 = 255;
  isKeyboardModeProfile3 = false;
  updateProfile3ToEEPROM();
}

void updateBoundsToEEPROM(short lowestX, short highestX, short lowestY, short highestY) {
  EEPROM.update(0, highByte(lowestX));
  EEPROM.update(1, lowByte(lowestX));
  EEPROM.update(2, highByte(highestX));
  EEPROM.update(3, lowByte(highestX));
  EEPROM.update(4, highByte(lowestY));
  EEPROM.update(5, lowByte(lowestY));
  EEPROM.update(6, highByte(highestY));
  EEPROM.update(7, lowByte(highestY));
}

void updateKeyboardModeOffsetsToEEPROM()
{
  EEPROM.update(8, keyboardModeXStartOffset);
  EEPROM.update(9, keyboardModeYStartOffset);
}

void updateProfile1ToEEPROM()
{
  EEPROM.update(10, keyProfile1[0][0]);
  EEPROM.update(11, keyProfile1[1][0]);
  EEPROM.update(12, keyProfile1[2][0]);
  EEPROM.update(13, keyProfile1[3][0]);
  EEPROM.update(14, keyProfile1[4][0]);
  EEPROM.update(15, keyProfile1[0][1]);
  EEPROM.update(16, keyProfile1[1][1]);
  EEPROM.update(17, keyProfile1[2][1]);
  EEPROM.update(18, keyProfile1[3][1]);
  EEPROM.update(19, keyProfile1[4][1]);
  EEPROM.update(20, keyProfile1[0][2]);
  EEPROM.update(21, keyProfile1[1][2]);
  EEPROM.update(22, keyProfile1[2][2]);
  EEPROM.update(23, keyProfile1[3][2]);
  EEPROM.update(24, keyProfile1[4][2]);
  EEPROM.update(25, keyProfile1[0][3]);
  EEPROM.update(26, keyProfile1[1][3]);
  EEPROM.update(27, keyProfile1[2][3]);
  EEPROM.update(28, keyProfile1[3][3]);
  EEPROM.update(29, keyProfile1[4][3]);
  EEPROM.update(30, joystickButtonProfile1);
  EEPROM.update(31, thumbButtonProfile1);
  EEPROM.update(32, keyboardModeStickProfile1[0]);
  EEPROM.update(33, keyboardModeStickProfile1[1]);
  EEPROM.update(34, keyboardModeStickProfile1[2]);
  EEPROM.update(35, keyboardModeStickProfile1[3]);
  EEPROM.update(36, (byte)keyProfileRGBRed1);
  EEPROM.update(37, (byte)keyProfileRGBGreen1);
  EEPROM.update(38, (byte)keyProfileRGBBlue1);
  EEPROM.update(97, dpadUpProfile1);
  EEPROM.update(98, dpadDownProfile1);
  EEPROM.update(99, dpadLeftProfile1);
  EEPROM.update(100, dpadRightProfile1);
  EEPROM.update(109, isKeyboardModeProfile1);
}

void updateProfile2ToEEPROM()
{
  EEPROM.update(39, keyProfile2[0][0]);
  EEPROM.update(40, keyProfile2[1][0]);
  EEPROM.update(41, keyProfile2[2][0]);
  EEPROM.update(42, keyProfile2[3][0]);
  EEPROM.update(43, keyProfile2[4][0]);
  EEPROM.update(44, keyProfile2[0][1]);
  EEPROM.update(45, keyProfile2[1][1]);
  EEPROM.update(46, keyProfile2[2][1]);
  EEPROM.update(47, keyProfile2[3][1]);
  EEPROM.update(48, keyProfile2[4][1]);
  EEPROM.update(49, keyProfile2[0][2]);
  EEPROM.update(50, keyProfile2[1][2]);
  EEPROM.update(51, keyProfile2[2][2]);
  EEPROM.update(52, keyProfile2[3][2]);
  EEPROM.update(53, keyProfile2[4][2]);
  EEPROM.update(54, keyProfile2[0][3]);
  EEPROM.update(55, keyProfile2[1][3]);
  EEPROM.update(56, keyProfile2[2][3]);
  EEPROM.update(57, keyProfile2[3][3]);
  EEPROM.update(58, keyProfile2[4][3]);
  EEPROM.update(59, joystickButtonProfile2);
  EEPROM.update(60, thumbButtonProfile2);
  EEPROM.update(61, keyboardModeStickProfile2[0]);
  EEPROM.update(62, keyboardModeStickProfile2[1]);
  EEPROM.update(63, keyboardModeStickProfile2[2]);
  EEPROM.update(64, keyboardModeStickProfile2[3]);
  EEPROM.update(65, (byte)keyProfileRGBRed2);
  EEPROM.update(66, (byte)keyProfileRGBGreen2);
  EEPROM.update(67, (byte)keyProfileRGBBlue2);
  EEPROM.update(101, dpadUpProfile2);
  EEPROM.update(102, dpadDownProfile2);
  EEPROM.update(103, dpadLeftProfile2);
  EEPROM.update(104, dpadRightProfile2);
  EEPROM.update(110, isKeyboardModeProfile2);
}

void updateProfile3ToEEPROM()
{
  EEPROM.update(68, keyProfile3[0][0]);
  EEPROM.update(69, keyProfile3[1][0]);
  EEPROM.update(70, keyProfile3[2][0]);
  EEPROM.update(71, keyProfile3[3][0]);
  EEPROM.update(72, keyProfile3[4][0]);
  EEPROM.update(73, keyProfile3[0][1]);
  EEPROM.update(74, keyProfile3[1][1]);
  EEPROM.update(75, keyProfile3[2][1]);
  EEPROM.update(76, keyProfile3[3][1]);
  EEPROM.update(77, keyProfile3[4][1]);
  EEPROM.update(78, keyProfile3[0][2]);
  EEPROM.update(79, keyProfile3[1][2]);
  EEPROM.update(80, keyProfile3[2][2]);
  EEPROM.update(81, keyProfile3[3][2]);
  EEPROM.update(82, keyProfile3[4][2]);
  EEPROM.update(83, keyProfile3[0][3]);
  EEPROM.update(84, keyProfile3[1][3]);
  EEPROM.update(85, keyProfile3[2][3]);
  EEPROM.update(86, keyProfile3[3][3]);
  EEPROM.update(87, keyProfile3[4][3]);
  EEPROM.update(88, joystickButtonProfile3);
  EEPROM.update(89, thumbButtonProfile3);
  EEPROM.update(90, keyboardModeStickProfile3[0]);
  EEPROM.update(91, keyboardModeStickProfile3[1]);
  EEPROM.update(92, keyboardModeStickProfile3[2]);
  EEPROM.update(93, keyboardModeStickProfile3[3]);
  EEPROM.update(94, (byte)keyProfileRGBRed3);
  EEPROM.update(95, (byte)keyProfileRGBGreen3);
  EEPROM.update(96, (byte)keyProfileRGBBlue3);
  EEPROM.update(105, dpadUpProfile3);
  EEPROM.update(106, dpadDownProfile3);
  EEPROM.update(107, dpadLeftProfile3);
  EEPROM.update(108, dpadRightProfile3);
  EEPROM.update(111, isKeyboardModeProfile3);
}

short getLowestXFromEEPROM() {
  return EEPROM.read(0) << 8 | EEPROM.read(1);
}

short getHighestXFromEEPROM() {
  return EEPROM.read(2) << 8 | EEPROM.read(3);
}

short getLowestYFromEEPROM() {
  return EEPROM.read(4) << 8 | EEPROM.read(5);
}

short getHighestYFromEEPROM() {
  return EEPROM.read(6) << 8 | EEPROM.read(7);
}

byte getKeyboardModeOffsetXFromEEPROM() {
  return EEPROM.read(8);
}

byte getKeyboardModeOffsetYFromEEPROM() {
  return EEPROM.read(9);
}

void setProfile1FromEEPROM()
{
  keyProfile1[0][0] = EEPROM.read(10);
  keyProfile1[1][0] = EEPROM.read(11);
  keyProfile1[2][0] = EEPROM.read(12);
  keyProfile1[3][0] = EEPROM.read(13);
  keyProfile1[4][0] = EEPROM.read(14);
  keyProfile1[0][1] = EEPROM.read(15);
  keyProfile1[1][1] = EEPROM.read(16);
  keyProfile1[2][1] = EEPROM.read(17);
  keyProfile1[3][1] = EEPROM.read(18);
  keyProfile1[4][1] = EEPROM.read(19);
  keyProfile1[0][2] = EEPROM.read(20);
  keyProfile1[1][2] = EEPROM.read(21);
  keyProfile1[2][2] = EEPROM.read(22);
  keyProfile1[3][2] = EEPROM.read(23);
  keyProfile1[4][2] = EEPROM.read(24);
  keyProfile1[0][3] = EEPROM.read(25);
  keyProfile1[1][3] = EEPROM.read(26);
  keyProfile1[2][3] = EEPROM.read(27);
  keyProfile1[3][3] = EEPROM.read(28);
  keyProfile1[4][3] = EEPROM.read(29);
  joystickButtonProfile1 = EEPROM.read(30);
  thumbButtonProfile1 = EEPROM.read(31);
  keyboardModeStickProfile1[0] = EEPROM.read(32);
  keyboardModeStickProfile1[1] = EEPROM.read(33);
  keyboardModeStickProfile1[2] = EEPROM.read(34);
  keyboardModeStickProfile1[3] = EEPROM.read(35);
  keyProfileRGBRed1 = (int)EEPROM.read(36);
  keyProfileRGBGreen1 = (int)EEPROM.read(37);
  keyProfileRGBBlue1 = (int)EEPROM.read(38);
  dpadUpProfile1 = EEPROM.read(97);
  dpadDownProfile1 = EEPROM.read(98);
  dpadLeftProfile1 = EEPROM.read(99);
  dpadRightProfile1 = EEPROM.read(100);
  isKeyboardModeProfile1 = (bool)EEPROM.read(109);
}

void setProfile2FromEEPROM()
{
  keyProfile2[0][0] = (char)EEPROM.read(39);
  keyProfile2[1][0] = (char)EEPROM.read(40);
  keyProfile2[2][0] = (char)EEPROM.read(41);
  keyProfile2[3][0] = (char)EEPROM.read(42);
  keyProfile2[4][0] = (char)EEPROM.read(43);
  keyProfile2[0][1] = (char)EEPROM.read(44);
  keyProfile2[1][1] = (char)EEPROM.read(45);
  keyProfile2[2][1] = (char)EEPROM.read(46);
  keyProfile2[3][1] = (char)EEPROM.read(47);
  keyProfile2[4][1] = (char)EEPROM.read(48);
  keyProfile2[0][2] = (char)EEPROM.read(49);
  keyProfile2[1][2] = (char)EEPROM.read(50);
  keyProfile2[2][2] = (char)EEPROM.read(51);
  keyProfile2[3][2] = (char)EEPROM.read(52);
  keyProfile2[4][2] = (char)EEPROM.read(53);
  keyProfile2[0][3] = (char)EEPROM.read(54);
  keyProfile2[1][3] = (char)EEPROM.read(55);
  keyProfile2[2][3] = (char)EEPROM.read(56);
  keyProfile2[3][3] = (char)EEPROM.read(57);
  keyProfile2[4][3] = (char)EEPROM.read(58);
  joystickButtonProfile2 = (char)EEPROM.read(59);
  thumbButtonProfile2 = (char)EEPROM.read(60);
  keyboardModeStickProfile2[0] = (char)EEPROM.read(61);
  keyboardModeStickProfile2[1] = (char)EEPROM.read(62);
  keyboardModeStickProfile2[2] = (char)EEPROM.read(63);
  keyboardModeStickProfile2[3] = (char)EEPROM.read(64);
  keyProfileRGBRed2 = (int)EEPROM.read(65);
  keyProfileRGBGreen2 = (int)EEPROM.read(66);
  keyProfileRGBBlue2 = (int)EEPROM.read(67);
  dpadUpProfile2 = EEPROM.read(101);
  dpadDownProfile2 = EEPROM.read(102);
  dpadLeftProfile2 = EEPROM.read(103);
  dpadRightProfile2 = EEPROM.read(104);
  isKeyboardModeProfile2 = (bool)EEPROM.read(110);
}

void setProfile3FromEEPROM()
{
  keyProfile3[0][0] = (char)EEPROM.read(68);
  keyProfile3[1][0] = (char)EEPROM.read(69);
  keyProfile3[2][0] = (char)EEPROM.read(70);
  keyProfile3[3][0] = (char)EEPROM.read(71);
  keyProfile3[4][0] = (char)EEPROM.read(72);
  keyProfile3[0][1] = (char)EEPROM.read(73);
  keyProfile3[1][1] = (char)EEPROM.read(74);
  keyProfile3[2][1] = (char)EEPROM.read(75);
  keyProfile3[3][1] = (char)EEPROM.read(76);
  keyProfile3[4][1] = (char)EEPROM.read(77);
  keyProfile3[0][2] = (char)EEPROM.read(78);
  keyProfile3[1][2] = (char)EEPROM.read(79);
  keyProfile3[2][2] = (char)EEPROM.read(80);
  keyProfile3[3][2] = (char)EEPROM.read(81);
  keyProfile3[4][2] = (char)EEPROM.read(82);
  keyProfile3[0][3] = (char)EEPROM.read(83);
  keyProfile3[1][3] = (char)EEPROM.read(84);
  keyProfile3[2][3] = (char)EEPROM.read(85);
  keyProfile3[3][3] = (char)EEPROM.read(86);
  keyProfile3[4][3] = (char)EEPROM.read(87);
  joystickButtonProfile3 = (char)EEPROM.read(88);
  thumbButtonProfile3 = (char)EEPROM.read(89);
  keyboardModeStickProfile3[0] = (char)EEPROM.read(90);
  keyboardModeStickProfile3[1] = (char)EEPROM.read(91);
  keyboardModeStickProfile3[2] = (char)EEPROM.read(92);
  keyboardModeStickProfile3[3] = (char)EEPROM.read(93);
  keyProfileRGBRed3 = (int)EEPROM.read(94);
  keyProfileRGBGreen3 = (int)EEPROM.read(95);
  keyProfileRGBBlue3 = (int)EEPROM.read(96);
  dpadUpProfile3 = EEPROM.read(105);
  dpadDownProfile3 = EEPROM.read(106);
  dpadLeftProfile3 = EEPROM.read(107);
  dpadRightProfile3 = EEPROM.read(108);
  isKeyboardModeProfile3 = (bool)EEPROM.read(111);
}

void receiveSerialData() {
  static byte endMarker = 10;
  byte receivedByte;
  int ndx = 0;
  bool invalid = false;

  memset(serialData, SERIAL_DATA_MAX_SIZE, sizeof(serialData));

  while (Serial.available() > 0) {
    receivedByte = Serial.read();
    //Serial.println(receivedByte);

    if (receivedByte == endMarker) {
      serialData[ndx] = endMarker;
      handleSerialRead();
      break;
    }

    serialData[ndx] = receivedByte;
    ndx++;

    if (ndx >= SERIAL_DATA_MAX_SIZE) {
      invalid = true;
      break;
    }
  }

  if (invalid) {
    writeSerialNotification('E', '0');
  }
}

void handleSerialRead() {
  switch (serialData[0]) {
    case 78:
      handleSerialNotification();
      break;
    case 71:
      handleSerialGet();
      break;
    case 83:
      handleSerialSet();
      break;
  }
}

void handleSerialNotification() {
  switch (serialData[1]) {
    case 0: // hi im the app
      writeSerialNotification('N', '0');
      break;
  }
}

void handleSerialGet() {
  switch (serialData[1]) {
    case 80:
      handleSerialGetProfile();
      break;
    case 65: // whats the current active profile?
      writeSerialSetValue('A', activeProfileNumber);
      break;
    case 66: // whats the current stick bounds
      writeSerialStickBoundary('X', 'L', getLowestXFromEEPROM());
      writeSerialStickBoundary('X', 'U', getHighestXFromEEPROM());
      writeSerialStickBoundary('Y', 'L', getLowestYFromEEPROM());
      writeSerialStickBoundary('Y', 'U', getHighestYFromEEPROM());
      break;
    case 67: // whats the current keyboard mode offsets
      writeSerialKeyboardModeOffset('X', keyboardModeXStartOffset);
      writeSerialKeyboardModeOffset('Y', keyboardModeYStartOffset);
      break;
    case 68: // is keyboard mode active?
      writeIsKeyboardModeActive();
      break;
  }
}

void handleSerialGetProfile() {
  switch (serialData[2]) {
    case 1:
      handleSerialGetProfile1();
      break;
    case 2:
      handleSerialGetProfile2();
      break;
    case 3:
      handleSerialGetProfile3();
      break;
  }
}

void handleSerialGetProfile1() {
  writeSerialProfileKey('1', 'a', keyProfile1[0][0]);
  writeSerialProfileKey('1', 'b', keyProfile1[1][0]);
  writeSerialProfileKey('1', 'c', keyProfile1[2][0]);
  writeSerialProfileKey('1', 'd', keyProfile1[3][0]);
  writeSerialProfileKey('1', 'e', keyProfile1[4][0]);
  writeSerialProfileKey('1', 'f', keyProfile1[0][1]);
  writeSerialProfileKey('1', 'g', keyProfile1[1][1]);
  writeSerialProfileKey('1', 'h', keyProfile1[2][1]);
  writeSerialProfileKey('1', 'i', keyProfile1[3][1]);
  writeSerialProfileKey('1', 'j', keyProfile1[4][1]);
  writeSerialProfileKey('1', 'k', keyProfile1[0][2]);
  writeSerialProfileKey('1', 'l', keyProfile1[1][2]);
  writeSerialProfileKey('1', 'm', keyProfile1[2][2]);
  writeSerialProfileKey('1', 'n', keyProfile1[3][2]);
  writeSerialProfileKey('1', 'o', keyProfile1[4][2]);
  writeSerialProfileKey('1', 'p', keyProfile1[0][3]);
  writeSerialProfileKey('1', 'q', keyProfile1[1][3]);
  writeSerialProfileKey('1', 'r', keyProfile1[2][3]);
  writeSerialProfileKey('1', 's', keyProfile1[3][3]);
  writeSerialProfileKey('1', 't', keyProfile1[4][3]);
  writeSerialProfileKey('1', 'u', joystickButtonProfile1);
  writeSerialProfileKey('1', 'v', thumbButtonProfile1);
  writeSerialProfileKey('1', 'w', keyboardModeStickProfile1[0]);
  writeSerialProfileKey('1', 'x', keyboardModeStickProfile1[1]);
  writeSerialProfileKey('1', 'y', keyboardModeStickProfile1[2]);
  writeSerialProfileKey('1', 'z', keyboardModeStickProfile1[3]);
  writeSerialProfileKey('1', '1', (byte)keyProfileRGBRed1);
  writeSerialProfileKey('1', '2', (byte)keyProfileRGBGreen1);
  writeSerialProfileKey('1', '3', (byte)keyProfileRGBBlue1);
  writeSerialProfileKey('1', '4', dpadUpProfile1);
  writeSerialProfileKey('1', '5', dpadDownProfile1);
  writeSerialProfileKey('1', '6', dpadLeftProfile1);
  writeSerialProfileKey('1', '7', dpadRightProfile1);
  writeSerialProfileKey('1', '8', isKeyboardModeProfile1);
}

void handleSerialGetProfile2() {
  writeSerialProfileKey('2', 'a', keyProfile2[0][0]);
  writeSerialProfileKey('2', 'b', keyProfile2[1][0]);
  writeSerialProfileKey('2', 'c', keyProfile2[2][0]);
  writeSerialProfileKey('2', 'd', keyProfile2[3][0]);
  writeSerialProfileKey('2', 'e', keyProfile2[4][0]);
  writeSerialProfileKey('2', 'f', keyProfile2[0][1]);
  writeSerialProfileKey('2', 'g', keyProfile2[1][1]);
  writeSerialProfileKey('2', 'h', keyProfile2[2][1]);
  writeSerialProfileKey('2', 'i', keyProfile2[3][1]);
  writeSerialProfileKey('2', 'j', keyProfile2[4][1]);
  writeSerialProfileKey('2', 'k', keyProfile2[0][2]);
  writeSerialProfileKey('2', 'l', keyProfile2[1][2]);
  writeSerialProfileKey('2', 'm', keyProfile2[2][2]);
  writeSerialProfileKey('2', 'n', keyProfile2[3][2]);
  writeSerialProfileKey('2', 'o', keyProfile2[4][2]);
  writeSerialProfileKey('2', 'p', keyProfile2[0][3]);
  writeSerialProfileKey('2', 'q', keyProfile2[1][3]);
  writeSerialProfileKey('2', 'r', keyProfile2[2][3]);
  writeSerialProfileKey('2', 's', keyProfile2[3][3]);
  writeSerialProfileKey('2', 't', keyProfile2[4][3]);
  writeSerialProfileKey('2', 'u', joystickButtonProfile2);
  writeSerialProfileKey('2', 'v', thumbButtonProfile2);
  writeSerialProfileKey('2', 'w', keyboardModeStickProfile2[0]);
  writeSerialProfileKey('2', 'x', keyboardModeStickProfile2[1]);
  writeSerialProfileKey('2', 'y', keyboardModeStickProfile2[2]);
  writeSerialProfileKey('2', 'z', keyboardModeStickProfile2[3]);
  writeSerialProfileKey('2', '1', (byte)keyProfileRGBRed2);
  writeSerialProfileKey('2', '2', (byte)keyProfileRGBGreen2);
  writeSerialProfileKey('2', '3', (byte)keyProfileRGBBlue2);
  writeSerialProfileKey('2', '4', dpadUpProfile2);
  writeSerialProfileKey('2', '5', dpadDownProfile2);
  writeSerialProfileKey('2', '6', dpadLeftProfile2);
  writeSerialProfileKey('2', '7', dpadRightProfile2);
  writeSerialProfileKey('2', '8', isKeyboardModeProfile2);
}

void handleSerialGetProfile3() {
  writeSerialProfileKey('3', 'a', keyProfile3[0][0]);
  writeSerialProfileKey('3', 'b', keyProfile3[1][0]);
  writeSerialProfileKey('3', 'c', keyProfile3[2][0]);
  writeSerialProfileKey('3', 'd', keyProfile3[3][0]);
  writeSerialProfileKey('3', 'e', keyProfile3[4][0]);
  writeSerialProfileKey('3', 'f', keyProfile3[0][1]);
  writeSerialProfileKey('3', 'g', keyProfile3[1][1]);
  writeSerialProfileKey('3', 'h', keyProfile3[2][1]);
  writeSerialProfileKey('3', 'i', keyProfile3[3][1]);
  writeSerialProfileKey('3', 'j', keyProfile3[4][1]);
  writeSerialProfileKey('3', 'k', keyProfile3[0][2]);
  writeSerialProfileKey('3', 'l', keyProfile3[1][2]);
  writeSerialProfileKey('3', 'm', keyProfile3[2][2]);
  writeSerialProfileKey('3', 'n', keyProfile3[3][2]);
  writeSerialProfileKey('3', 'o', keyProfile3[4][2]);
  writeSerialProfileKey('3', 'p', keyProfile3[0][3]);
  writeSerialProfileKey('3', 'q', keyProfile3[1][3]);
  writeSerialProfileKey('3', 'r', keyProfile3[2][3]);
  writeSerialProfileKey('3', 's', keyProfile3[3][3]);
  writeSerialProfileKey('3', 't', keyProfile3[4][3]);
  writeSerialProfileKey('3', 'u', joystickButtonProfile3);
  writeSerialProfileKey('3', 'v', thumbButtonProfile3);
  writeSerialProfileKey('3', 'w', keyboardModeStickProfile3[0]);
  writeSerialProfileKey('3', 'x', keyboardModeStickProfile3[1]);
  writeSerialProfileKey('3', 'y', keyboardModeStickProfile3[2]);
  writeSerialProfileKey('3', 'z', keyboardModeStickProfile3[3]);
  writeSerialProfileKey('3', '1', (byte)keyProfileRGBRed3);
  writeSerialProfileKey('3', '2', (byte)keyProfileRGBGreen3);
  writeSerialProfileKey('3', '3', (byte)keyProfileRGBBlue3);
  writeSerialProfileKey('3', '4', dpadUpProfile3);
  writeSerialProfileKey('3', '5', dpadDownProfile3);
  writeSerialProfileKey('3', '6', dpadLeftProfile3);
  writeSerialProfileKey('3', '7', dpadRightProfile3);
  writeSerialProfileKey('3', '8', isKeyboardModeProfile3);
}

void handleSerialSet() {
  switch (serialData[1]) {
    case 65:
      handleSerialSetCurrentProfile();
      break;
    case 66:
      handleSerialSetCurrentStickBounds();
      break;
    case 67:
      handleSerialSetKeyboardModeOffsets();
      break;
    case 68:
      handleSerialPersistValues();
      break;
    case 69:
      handleSerialInitProfiles();
      break;
    case 70:
      handleSerialSetKeyboardIsActive();
      break;
    case 80:
      handleSerialSetProfile();
      break;
  }
}

void handleSerialSetProfile() {
  switch (serialData[2]) {
    case 1:
      handleSerialSetProfile1();

      if (activeProfileNumber == 1) {
        handleSerialSetActiveProfile();
      }
      break;
    case 2:
      handleSerialSetProfile2();

      if (activeProfileNumber == 2) {
        handleSerialSetActiveProfile();
      }
      break;
    case 3:
      handleSerialSetProfile3();

      if (activeProfileNumber == 3) {
        handleSerialSetActiveProfile();
      }
      break;
  }
}

void handleSerialSetCurrentProfile() {
  switch (serialData[2]) {
    case 1:
      switchProfile(switchProfile1Key);
      break;
    case 2:
      switchProfile(switchProfile2Key);
      break;
    case 3:
      switchProfile(switchProfile3Key);
      break;
  }
}

void handleSerialSetCurrentStickBounds() {
  if (serialData[2] == 120) { // x axis
    if (serialData[3] == 108) { // lower
      xLow = serialData[4] << 8 | serialData[5];
    } else { // upper
      xHigh = serialData[4] << 8 | serialData[5];
    }
  } else { // y axis
    if (serialData[3] == 108) { // lower
      yLow = serialData[4] << 8 | serialData[5];
    } else { // upper
      yHigh = serialData[4] << 8 | serialData[5];
    }
  }
}

void handleSerialSetKeyboardModeOffsets() {
  if (serialData[2] == 120) { // x axis
    keyboardModeXStartOffset = serialData[3];
  } else { // y axis
    keyboardModeYStartOffset = serialData[3];
  }
}

void handleSerialPersistValues() {
  updateBoundsToEEPROM(xLow, xHigh, yLow, yHigh);
  updateKeyboardModeOffsetsToEEPROM();
  updateProfile1ToEEPROM();
  updateProfile2ToEEPROM();
  updateProfile3ToEEPROM();
}

void handleSerialInitProfiles() {
  initEEPROM();
  setProfile1FromEEPROM();
  setProfile2FromEEPROM();
  setProfile3FromEEPROM();
  originalHoldProfileKey = switchProfile1Key;
  switchProfile(switchProfile1Key);
  handleSerialGetProfile1();
  handleSerialGetProfile2();
  handleSerialGetProfile3();
  writeSerialSetValue('A', activeProfileNumber);
  writeSerialStickBoundary('X', 'L', getLowestXFromEEPROM());
  writeSerialStickBoundary('X', 'U', getHighestXFromEEPROM());
  writeSerialStickBoundary('Y', 'L', getLowestYFromEEPROM());
  writeSerialStickBoundary('Y', 'U', getHighestYFromEEPROM());
  writeSerialKeyboardModeOffset('X', keyboardModeXStartOffset);
  writeSerialKeyboardModeOffset('Y', keyboardModeYStartOffset);
}

void handleSerialSetActiveProfile() {
  switch (serialData[3]) {
    case 97:
      activeKeyProfile[0][0] = serialData[4];
      break;
    case 98:
      activeKeyProfile[1][0] = serialData[4];
      break;
    case 99:
      activeKeyProfile[2][0] = serialData[4];
      break;
    case 100:
      activeKeyProfile[3][0] = serialData[4];
      break;
    case 101:
      activeKeyProfile[4][0] = serialData[4];
      break;
    case 102:
      activeKeyProfile[0][1] = serialData[4];
      break;
    case 103:
      activeKeyProfile[1][1] = serialData[4];
      break;
    case 104:
      activeKeyProfile[2][1] = serialData[4];
      break;
    case 105:
      activeKeyProfile[3][1] = serialData[4];
      break;
    case 106:
      activeKeyProfile[4][1] = serialData[4];
      break;
    case 107:
      activeKeyProfile[0][2] = serialData[4];
      break;
    case 108:
      activeKeyProfile[1][2] = serialData[4];
      break;
    case 109:
      activeKeyProfile[2][2] = serialData[4];
      break;
    case 110:
      activeKeyProfile[3][2] = serialData[4];
      break;
    case 111:
      activeKeyProfile[4][2] = serialData[4];
      break;
    case 112:
      activeKeyProfile[0][3] = serialData[4];
      break;
    case 113:
      activeKeyProfile[1][3] = serialData[4];
      break;
    case 114:
      activeKeyProfile[2][3] = serialData[4];
      break;
    case 115:
      activeKeyProfile[3][3] = serialData[4];
      break;
    case 116:
      activeKeyProfile[4][3] = serialData[4];
      break;
    case 119:
      activekeyboardModeStickProfile[0] = serialData[4];
      break;
    case 120:
      activekeyboardModeStickProfile[1] = serialData[4];
      break;
    case 121:
      activekeyboardModeStickProfile[2] = serialData[4];
      break;
    case 122:
      activekeyboardModeStickProfile[3] = serialData[4];
      break;
  }
}

void handleSerialSetProfile1() {
  bool isRgbValue = false;
  
  switch (serialData[3]) {
    case 97:
      keyProfile1[0][0] = serialData[4];
      break;
    case 98:
      keyProfile1[1][0] = serialData[4];
      break;
    case 99:
      keyProfile1[2][0] = serialData[4];
      break;
    case 100:
      keyProfile1[3][0] = serialData[4];
      break;
    case 101:
      keyProfile1[4][0] = serialData[4];
      break;
    case 102:
      keyProfile1[0][1] = serialData[4];
      break;
    case 103:
      keyProfile1[1][1] = serialData[4];
      break;
    case 104:
      keyProfile1[2][1] = serialData[4];
      break;
    case 105:
      keyProfile1[3][1] = serialData[4];
      break;
    case 106:
      keyProfile1[4][1] = serialData[4];
      break;
    case 107:
      keyProfile1[0][2] = serialData[4];
      break;
    case 108:
      keyProfile1[1][2] = serialData[4];
      break;
    case 109:
      keyProfile1[2][2] = serialData[4];
      break;
    case 110:
      keyProfile1[3][2] = serialData[4];
      break;
    case 111:
      keyProfile1[4][2] = serialData[4];
      break;
    case 112:
      keyProfile1[0][3] = serialData[4];
      break;
    case 113:
      keyProfile1[1][3] = serialData[4];
      break;
    case 114:
      keyProfile1[2][3] = serialData[4];
      break;
    case 115:
      keyProfile1[3][3] = serialData[4];
      break;
    case 116:
      keyProfile1[4][3] = serialData[4];
      break;
    case 117:
      joystickButtonProfile1 = serialData[4];
      break;
    case 118:
      thumbButtonProfile1 = serialData[4];
      break;
    case 119:
      keyboardModeStickProfile1[0] = serialData[4];
      break;
    case 120:
      keyboardModeStickProfile1[1] = serialData[4];
      break;
    case 121:
      keyboardModeStickProfile1[2] = serialData[4];
      break;
    case 122:
      keyboardModeStickProfile1[3] = serialData[4];
      break;
    case 49:
      keyProfileRGBRed1 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 50:
      keyProfileRGBGreen1 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 51:
      keyProfileRGBBlue1 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 52:
      dpadUpProfile1 = serialData[4];
      break;
    case 53:
      dpadDownProfile1 = serialData[4];
      break;
    case 54:
      dpadLeftProfile1 = serialData[4];
      break;
    case 55:
      dpadRightProfile1 = serialData[4];
      break;
    case 123:
      isKeyboardModeProfile1 = (bool)serialData[4];

      if (isKeyboardModeProfile1) {
        isKeyboardMode = true;
      } else {
        isKeyboardMode = false;
        setRGBColorToActiveProfile();
      }
      
      break;
  }

  if (activeProfileNumber == 1 && isRgbValue) {
    setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
  }
}

void handleSerialSetProfile2() {
  bool isRgbValue = false;
  
  switch (serialData[3]) {
    case 97:
      keyProfile2[0][0] = serialData[4];
      break;
    case 98:
      keyProfile2[1][0] = serialData[4];
      break;
    case 99:
      keyProfile2[2][0] = serialData[4];
      break;
    case 100:
      keyProfile2[3][0] = serialData[4];
      break;
    case 101:
      keyProfile2[4][0] = serialData[4];
      break;
    case 102:
      keyProfile2[0][1] = serialData[4];
      break;
    case 103:
      keyProfile2[1][1] = serialData[4];
      break;
    case 104:
      keyProfile2[2][1] = serialData[4];
      break;
    case 105:
      keyProfile2[3][1] = serialData[4];
      break;
    case 106:
      keyProfile2[4][1] = serialData[4];
      break;
    case 107:
      keyProfile2[0][2] = serialData[4];
      break;
    case 108:
      keyProfile2[1][2] = serialData[4];
      break;
    case 109:
      keyProfile2[2][2] = serialData[4];
      break;
    case 110:
      keyProfile2[3][2] = serialData[4];
      break;
    case 111:
      keyProfile2[4][2] = serialData[4];
      break;
    case 112:
      keyProfile2[0][3] = serialData[4];
      break;
    case 113:
      keyProfile2[1][3] = serialData[4];
      break;
    case 114:
      keyProfile2[2][3] = serialData[4];
      break;
    case 115:
      keyProfile2[3][3] = serialData[4];
      break;
    case 116:
      keyProfile2[4][3] = serialData[4];
      break;
    case 117:
      joystickButtonProfile2 = serialData[4];
      break;
    case 118:
      thumbButtonProfile2 = serialData[4];
      break;
    case 119:
      keyboardModeStickProfile2[0] = serialData[4];
      break;
    case 120:
      keyboardModeStickProfile2[1] = serialData[4];
      break;
    case 121:
      keyboardModeStickProfile2[2] = serialData[4];
      break;
    case 122:
      keyboardModeStickProfile2[3] = serialData[4];
      break;
    case 49:
      keyProfileRGBRed2 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 50:
      keyProfileRGBGreen2 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 51:
      keyProfileRGBBlue2 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 52:
      dpadUpProfile2 = serialData[4];
      break;
    case 53:
      dpadDownProfile2 = serialData[4];
      break;
    case 54:
      dpadLeftProfile2 = serialData[4];
      break;
    case 55:
      dpadRightProfile2 = serialData[4];
      break;
    case 123:
      isKeyboardModeProfile2 = (bool)serialData[4];

      if (isKeyboardModeProfile2) {
        isKeyboardMode = true;
      } else {
        isKeyboardMode = false;
        setRGBColorToActiveProfile();
      }
      
      break;
  }

  if (activeProfileNumber == 2 && isRgbValue) {
    setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
  }
}

void handleSerialSetProfile3() {
  bool isRgbValue = false;
  
  switch (serialData[3]) {
    case 97:
      keyProfile3[0][0] = serialData[4];
      break;
    case 98:
      keyProfile3[1][0] = serialData[4];
      break;
    case 99:
      keyProfile3[2][0] = serialData[4];
      break;
    case 100:
      keyProfile3[3][0] = serialData[4];
      break;
    case 101:
      keyProfile3[4][0] = serialData[4];
      break;
    case 102:
      keyProfile3[0][1] = serialData[4];
      break;
    case 103:
      keyProfile3[1][1] = serialData[4];
      break;
    case 104:
      keyProfile3[2][1] = serialData[4];
      break;
    case 105:
      keyProfile3[3][1] = serialData[4];
      break;
    case 106:
      keyProfile3[4][1] = serialData[4];
      break;
    case 107:
      keyProfile3[0][2] = serialData[4];
      break;
    case 108:
      keyProfile3[1][2] = serialData[4];
      break;
    case 109:
      keyProfile3[2][2] = serialData[4];
      break;
    case 110:
      keyProfile3[3][2] = serialData[4];
      break;
    case 111:
      keyProfile3[4][2] = serialData[4];
      break;
    case 112:
      keyProfile3[0][3] = serialData[4];
      break;
    case 113:
      keyProfile3[1][3] = serialData[4];
      break;
    case 114:
      keyProfile3[2][3] = serialData[4];
      break;
    case 115:
      keyProfile3[3][3] = serialData[4];
      break;
    case 116:
      keyProfile3[4][3] = serialData[4];
      break;
    case 117:
      joystickButtonProfile3 = serialData[4];
      break;
    case 118:
      thumbButtonProfile3 = serialData[4];
      break;
    case 119:
      keyboardModeStickProfile3[0] = serialData[4];
      break;
    case 120:
      keyboardModeStickProfile3[1] = serialData[4];
      break;
    case 121:
      keyboardModeStickProfile3[2] = serialData[4];
      break;
    case 122:
      keyboardModeStickProfile3[3] = serialData[4];
      break;
    case 49:
      keyProfileRGBRed3 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 50:
      keyProfileRGBGreen3 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 51:
      keyProfileRGBBlue3 = (int)serialData[4];
      isRgbValue = true;
      break;
    case 52:
      dpadUpProfile3 = serialData[4];
      break;
    case 53:
      dpadDownProfile3 = serialData[4];
      break;
    case 54:
      dpadLeftProfile3 = serialData[4];
      break;
    case 55:
      dpadRightProfile3 = serialData[4];
      break;
    case 123:
      isKeyboardModeProfile3 = (bool)serialData[4];

      if (isKeyboardModeProfile3) {
        isKeyboardMode = true;
      } else {
        isKeyboardMode = false;
        setRGBColorToActiveProfile();
      }
      
      break;
  }

  if (activeProfileNumber == 3 && isRgbValue) {
    setRGBLed(keyProfileRGBRed1, keyProfileRGBGreen1, keyProfileRGBBlue1);
  }
}

void handleSerialSetKeyboardIsActive() {
  if (serialData[2] == 1) {
    isKeyboardMode = true;
  } else {
    isKeyboardMode = false;
    setRGBColorToActiveProfile();
  }
}

void writeSerialProfileKey(char profileNumber, char key, byte value) {
  Serial.print('S');
  Serial.print('P');
  Serial.print(profileNumber);
  Serial.print(key);
  Serial.println(value);
}

void writeSerialSetValue(char key, byte value) {
  Serial.print('S');
  Serial.print(key);
  Serial.println(value);
}

void writeSerialNotification(char code, char value) {
  Serial.print(code);
  Serial.println(value);
}

void writeSerialStickBoundary(char axis, char dir, short value) {
  Serial.print('S');
  Serial.print('B');
  Serial.print(axis);
  Serial.print(dir);
  Serial.println(value);
}

void writeSerialKeyboardModeOffset(char axis, byte value) {
  Serial.print('S');
  Serial.print('C');
  Serial.print(axis);
  Serial.println(value);
}

void writeIsKeyboardModeActive() {
  Serial.print('S');
  Serial.print('D');
  Serial.println(isKeyboardMode);
}

void writeCurrentActiveProfile() {
  Serial.print('S');
  Serial.print('A');
  Serial.println(activeProfileNumber);
}