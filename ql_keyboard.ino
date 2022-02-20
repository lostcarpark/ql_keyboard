

/*
 * Key status values:
 * Key press:   debounce*2
 * Key down:    debounce+1 to debounce*2-1
 * Key release: debounce
 * Key up:      0 to debounce-1
 */
const int debounce = 16;
const int debounceBy2 = debounce * 2;
const int debouncePlus1 = debounce + 1;

const int KEY_EUROPE_1 = 0xf032;  // European 102 key codes not in predefined keycodes.
const int KEY_EUROPE_2 = 0xf064;

const int ledPin = 13;

const byte modifierRow = 22;
const byte modifierCols[] = {11, 10, 1}; // Ctrl, Shift, Alt.
const byte modifierCtrl = 0;  // Modifier positions in array.
const byte modifierShift = 1;
const byte modifierAlt = 2;
const int modifierCodes[] = {MODIFIERKEY_CTRL, MODIFIERKEY_SHIFT, MODIFIERKEY_ALT};
const int modifierCount = sizeof(modifierCols);
int modifierStates[modifierCount];

bool backSpace = false;

byte rowPins[] = {21, 20, 19, 18, 17, 16, 15, 14};
const int rowCount = sizeof(rowPins)/sizeof(rowPins[0]);
 
byte colPins[] = {9,8,7,6,5,4,3,2};
const int colCount = sizeof(colPins)/sizeof(colPins[0]);

int keyCodes[rowCount][colCount] = {
  { KEY_LEFT, KEY_ESC, KEY_RIGHT, KEY_SPACE, KEY_UP, KEY_DOWN, KEY_RETURN, KEY_EUROPE_2 },
  { KEY_SPACE, KEY_X, KEY_V, KEY_N, KEY_SPACE, KEY_COMMA, KEY_SPACE, KEY_SLASH },
  { KEY_Z, KEY_C, KEY_B, KEY_M, KEY_PERIOD, KEY_QUOTE, KEY_RIGHT_BRACE, KEY_EUROPE_1 },
  { KEY_CAPS_LOCK, KEY_S, KEY_F, KEY_G, KEY_K, KEY_SEMICOLON, KEY_LEFT_BRACE, KEY_EQUAL },
  { KEY_3, KEY_1, KEY_A, KEY_D, KEY_H, KEY_J, KEY_L, KEY_P },
  { KEY_W, KEY_TAB, KEY_R, KEY_Y, KEY_I, KEY_O, KEY_9, KEY_MINUS },
  { KEY_2, KEY_Q, KEY_E, KEY_T, KEY_6, KEY_U, KEY_8, KEY_0 },
  { KEY_F1, KEY_F2, KEY_F3, KEY_4, KEY_5, KEY_7, KEY_F4, KEY_F5 }
};

int keyStates[rowCount][colCount];

/*
 * Called for initialisation. Sets up pins for keyboard mapping.
 */
void setup() {
  pinMode(ledPin, OUTPUT);

  // Set up row pin for modifier row.
  pinMode(modifierRow, OUTPUT);
  digitalWrite(modifierRow, LOW);
  pinMode(modifierRow, INPUT);

  // Set up col pins for modifier row.
  for (int i = 0; i < modifierCount; i++)
    pinMode(modifierCols[i], INPUT_PULLUP);

  // Set up row pins for regular keys.
  for (int i = 0; i < rowCount; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
    pinMode(rowPins[i], INPUT);
  }

  // Set up col pins for regular keys.
  for (int i = 0; i < colCount; i++)
    pinMode(colPins[i], INPUT_PULLUP);

  // Initialise all states to 0;
  for (int i = 0; i < modifierCount; i++)
    modifierStates[i] = 0;

  for (int i = 0; i < rowCount; i++)
    for (int j = 0; j < colCount; j++)
      keyStates[i][j] = 0;
}


void keyPress(int keyCode) {
  // Check for CTRL+LEFT.
  if (keyCode == KEY_LEFT && modifierStates[modifierCtrl] > debouncePlus1) {
    // Release ctrl and press backspace key.
    if (!backSpace) {
      // If not already in backspace, release CTRL key.
      Keyboard.release(MODIFIERKEY_CTRL);
      delayMicroseconds(200);
    }
    Keyboard.press(KEY_BACKSPACE);
    backSpace = true;
    return;
  }

  // Ignore any CTRL changes while in backspace.
  if (keyCode == MODIFIERKEY_CTRL && backSpace) {
    return;
  }

  Keyboard.press(keyCode);
}


void keyRelease(int keyCode) {
  // Check if left released and in backspace.
  if (keyCode == KEY_LEFT && backSpace) {
    Keyboard.release(KEY_BACKSPACE);
    if (modifierStates[modifierCtrl] > debouncePlus1) {
      // If CTRL still pressed, resend CTRL press after suitable delay.
      delayMicroseconds(200);
      Keyboard.press(MODIFIERKEY_CTRL);
    }
    backSpace = false;
    return;
  }

  // Ignore releasing CTRL while in backspace.
  if (keyCode == MODIFIERKEY_CTRL && backSpace) {
    return;
  }

  Keyboard.release(keyCode);
}


/*
 * Scan a key and check for keybounce.
 * Params:
 *   pin: pin number to read.
 *   state: current stage value from 0 to debounce*2.
 *   keyCode: Key code to send if pressed.
 * Returns:
 *   New state value.
 */
int scanKey(int pin, int state, int keyCode) {
  int pinVal = digitalRead(pin);
  // If state is zero, and key down, generate keypress.
  if (state == 0 && pinVal == LOW) {
    // Key press event.
    keyPress(keyCode);
    return debounceBy2;
  }

  // If state is debounce + 1 and key released, generate key release.
  if (state == debouncePlus1) {
    if (pinVal == HIGH) {
      keyRelease(keyCode);
      return state - 1;
    }

    // Key still pressed, so send keypress again, and go back to debounce*2.
    keyPress(keyCode);
    return debounceBy2;
  }

  // State is positive, so decrement.
  if (state > 0)
    return state - 1;

  // Default action: if we get here, state should be zero, so always return 0.
  return 0;
}


/*
 * Scan a keyboard row.
 * Params:
 *   rowPin: The Teensy pin to output.
 *   colPins: Array of pins to read from.
 *   count: The number of input pins to scan.
 *   state: Array of key state values.
 *   codes: Array of keyboard scancodes.
 * Returns:
 *   Nothing.
 */
void scanRow(int rowPin, const byte *colPins, int count, int *state, const int *codes) {
  pinMode(rowPin, OUTPUT);
  digitalWrite(rowPin, LOW);
  delayMicroseconds(200);

  for (int i=0; i<count; i++) {
    state[i] = scanKey(colPins[i], state[i], codes[i]);
  }
  pinMode(rowPin, INPUT);
}


/*
 * Main program loop. Runs repeatedly.
 */
void loop() {
  scanRow(modifierRow, modifierCols, modifierCount, modifierStates, modifierCodes);
  for (int row = 0; row < rowCount; row++)
    scanRow(rowPins[row], colPins, colCount, keyStates[row], keyCodes[row]);

  // Just for debugging, turn on LED if any of the modifier keys pressed.
  if (modifierStates[modifierCtrl] || modifierStates[modifierShift] || modifierStates[modifierAlt])
  //if (keyStates[6][0])
    digitalWrite(ledPin, HIGH);
  else
    digitalWrite(ledPin, LOW);
  delay(5);
}
