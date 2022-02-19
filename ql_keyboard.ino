

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

const int ledPin = 13;

const byte modifierRow = 22;
const byte modifierCols[] = {11, 10, 1}; // Ctrl, Shift, Alt.
const int modifierCodes[] = {MODIFIERKEY_CTRL, MODIFIERKEY_SHIFT, MODIFIERKEY_ALT};
const int modifierCount = sizeof(modifierCols);
int modifierStates[modifierCount];

byte rowPins[] = {21, 20, 19, 18, 17, 16, 15, 14};
const int rowCount = sizeof(rowPins)/sizeof(rowPins[0]);
 
byte colPins[] = {9,8,7,6,5,4,3,2};
const int colCount = sizeof(colPins)/sizeof(colPins[0]);

int keyCodes[rowCount][colCount] = {
  { KEY_LEFT, KEY_ESC, KEY_RIGHT, KEY_SPACE, KEY_UP, KEY_DOWN, KEY_RETURN, KEY_TILDE },
  { KEY_SPACE, KEY_X, KEY_V, KEY_N, KEY_SPACE, KEY_COMMA, KEY_SPACE, KEY_SLASH },
  { KEY_Z, KEY_C, KEY_B, KEY_M, KEY_PERIOD, KEY_QUOTE, KEY_RIGHT_BRACE, KEY_BACKSLASH },
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
    Keyboard.press(keyCode);
    return debounceBy2;
  }

  // If state is debounce + 1 and key released, generate key release.
  if (state == debouncePlus1) {
    if (pinVal == HIGH) {
      Keyboard.release(keyCode);
      return state - 1;
    }

    // Key still pressed, so send keypress again, and go back to debounce*2.
    Keyboard.press(keyCode);
    return debounceBy2;
  }

  // State is positive, so decrement.
  if (state > 0)
    return state - 1;

  // Default action: if we get here, state should be zero, so always return 0.
  return 0;
}

int scanRow(int rowPin, byte *colPins, int count, int *state, int *codes) {
  pinMode(rowPin, OUTPUT);
  digitalWrite(rowPin, LOW);
  delayMicroseconds(200);

  for (int i=0; i<count; i++) {
    state[i] = scanKey(colPins[i], state[i], codes[i]);
  }
  pinMode(rowPin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  scanRow(modifierRow, modifierCols, modifierCount, modifierStates, modifierCodes);
  for (int row = 0; row < rowCount; row++)
    scanRow(rowPins[row], colPins, colCount, keyStates[row], keyCodes[row]);

  if (modifierStates[0] || modifierStates[1] || modifierStates[2])
  //if (keyStates[6][0])
    digitalWrite(ledPin, HIGH);
  else
    digitalWrite(ledPin, LOW);
  delay(5);
}
