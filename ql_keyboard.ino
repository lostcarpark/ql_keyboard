

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

// USB keyboard can send up to 6 pressed keys. The 6 most recent keys are stored on a stack.
const int stackSize = 6;
int keyStack[stackSize];

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

  for (int i = 0; i < stackSize; i++)
    keyStack[i] = 0;
}

/*
 * Check if key is on stack.
 * Params:
 *   keyCode: the key scancode to check.
 * Returns:
 *   True if key on stack, otherwise false.
 */
bool checkStack(int keyCode) {
  for (int i = 0; i < stackSize; i++)
    if (keyStack[i] == keyCode)
      return true;
  return false;
}

/*
 * Add key to stack.
 * Advance any keys already on stack to the next slot, and add new key to start,
 * Params:
 *   keyCode: the key scancode to add.
 * Returns:
 *   Nothing.
 */
void addToStack(int keyCode) {
  if (checkStack(keyCode))
    return;   // Key is already on stack, so nothing to do.

  // Advance any keys on the stack by one position, and add the new key to the front.
  for (int i = stackSize - 1; i > 0; i--)
    keyStack[i] = keyStack[i - 1];

  keyStack[0] = keyCode;
}

/*
 * Remove key from stack.
 * If key scancode on stack, remove it. Move any other keys up to take its place.
 * Params:
 *   keyCode: the key scancode to remove.
 * Returns:
 *   Nothing.
 */
void removeFromStack(int keyCode) {
  for (int i = 0; i < stackSize; i++) {
    if (keyStack[i] == keyCode)
      keyStack[i] = 0;

    if (keyStack[i] == 0 && i < stackSize - 1 && keyStack[i + 1] != 0) {
      int temp = keyStack[i];
      keyStack[i] = keyStack[i + 1];
      keyStack[i + 1] = temp;
    }
  }
}

/*
 * Combine any pressed modifiers with OR operator.
 */
int combineModifiers(bool suppressCtrl, bool suppressAlt, bool sendRightAlt, bool sendGUI) {
  int combined = 0;
  if (!suppressCtrl && modifierStates[modifierCtrl] > debouncePlus1)
    combined |= MODIFIERKEY_CTRL;
  if (modifierStates[modifierShift] > debouncePlus1)
    combined |= MODIFIERKEY_SHIFT;
  if (!suppressAlt && modifierStates[modifierAlt] > debouncePlus1)
    combined |= MODIFIERKEY_ALT;
  if (sendRightAlt)
    combined |= MODIFIERKEY_RIGHT_ALT;
  if (sendGUI)
    combined |= MODIFIERKEY_GUI;
  return combined;
}

/*
 * Send all pressed keys on stack with modifiers.
 * Params:
 *   suppressCtrl: If true, don't send CTRL modifier.
 *   suppressAlt: If true, don't send ALT modifier.
 *   sendRightAlt: If true, send with right ALT modifier.
 *   sendSuper: If true, send with SUPER modifier.
 * Returns:
 *   Nothing.
 */
void sendKeyStack(bool suppressCtrl = false, bool suppressAlt = false, bool sendRightAlt = false, bool sendGUI = false) {
  Keyboard.set_key1(keyStack[0]);
  Keyboard.set_key2(keyStack[1]);
  Keyboard.set_key3(keyStack[2]);
  Keyboard.set_key4(keyStack[3]);
  Keyboard.set_key5(keyStack[4]);
  Keyboard.set_key6(keyStack[5]);
  Keyboard.set_modifier(combineModifiers(suppressCtrl, suppressAlt, sendRightAlt, sendGUI));
  Keyboard.send_now();
}


/*
 * Handle keypres.
 * Modify sent key if required.
 * Params:
 *   keyCode: Keypress scan code.
 * Returns:
 *   Nothing.
 */
void keyPress(int keyCode) {
  // Check for CTRL+ALT+RIGHT and change to CTRL+ALT+DEL.
  if (keyCode == KEY_RIGHT && modifierStates[modifierCtrl] > debouncePlus1 && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(KEY_DELETE);  // Send delete instead of right.
    sendKeyStack(false, false);  // Don't suppress modifiers - will send CTRL+ALT+DEL.
    return;
  }

  // Check for ALT+F5 and change to "Super" key.
  if (keyCode == KEY_F5 && modifierStates[modifierAlt] > debouncePlus1) {
    sendKeyStack(false, true, false, true);    // Send keys. Suppress alt key and force super key.
    return;
  }

  // Check for CTRL+LEFT and change to BACKSPACE.
  if (keyCode == KEY_LEFT && modifierStates[modifierCtrl] > debouncePlus1) {
    addToStack(KEY_BACKSPACE);  // Send backspace instead of left.
    sendKeyStack(true);    // Send keys, but suppress CTRL.
    return;
  }

  // Check for CTRL+RIGHT and change to DELETE (right).
  if (keyCode == KEY_RIGHT && modifierStates[modifierCtrl] > debouncePlus1) {
    addToStack(KEY_DELETE);  // Send delete instead of right.
    sendKeyStack(true);
    return;
  }

  // Check for ALT+LEFT and change to HOME.
  if (keyCode == KEY_LEFT && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(KEY_HOME);  // Send home instead of left.
    sendKeyStack(false, true);    // Send keys, but suppress alt.
    return;
  }

  // Check for ALT+RIGHT and change to END.
  if (keyCode == KEY_RIGHT && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(KEY_END);  // Send end instead of right.
    sendKeyStack(false, true);    // Send keys, but suppress alt.
    return;
  }

  // Check for ALT+UP and change to PAGE UP.
  if (keyCode == KEY_UP && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(KEY_PAGE_UP);  // Send page up instead of up.
    sendKeyStack(false, true);    // Send keys, but suppress alt.
    return;
  }

  // Check for ALT+DOWN and change to PAGE DOWN.
  if (keyCode == KEY_DOWN && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(KEY_PAGE_DOWN);  // Send page down instead of down.
    sendKeyStack(false, true);    // Send keys, but suppress alt.
    return;
  }

  // If ALT + 4 pressed, change modifier to right alt for Euro sign.
  if (keyCode == KEY_4 && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(keyCode);
    sendKeyStack(false, true, true);    // Send keys, but suppress alt and send right alt.
    return;
  }  

  // If ALT + AEIOU pressed, change modifier to right alt for Irish accents.
  if ((keyCode == KEY_A || keyCode == KEY_E || keyCode == KEY_I || keyCode == KEY_O || keyCode == KEY_U)
      && modifierStates[modifierAlt] > debouncePlus1) {
    addToStack(keyCode);
    sendKeyStack(false, true, true);    // Send keys, but suppress alt and send right alt.
    return;
  }  

  // If modifier key was pressed, do not add to stack, but send key stack.
  if (keyCode == MODIFIERKEY_CTRL || keyCode == MODIFIERKEY_SHIFT || keyCode == MODIFIERKEY_ALT) {
    sendKeyStack(false, true);    // Send keys, but suppress alt.
    return;
  }

  // If we've got here, just a normal keypress. Add to stack and send.
  addToStack(keyCode);
  sendKeyStack();
}

/*
 * Handle key release, including release of modified keys.
 * Params:
 *   keyCode: scan code of released key.
 * Returns:
 *   Nothing.
 */
void keyRelease(int keyCode) {
  if (keyCode == KEY_F5 && modifierStates[modifierAlt] > debouncePlus1) {
    sendKeyStack(false, true, false, false);    // Send keys. Suppress alt key and release super key (important as Super triggers on release).
    return;
  }

  // Check if left released and in backspace.
  if (keyCode == KEY_LEFT) {
    removeFromStack(KEY_BACKSPACE); // Left arrow released. Remove backspace from stack as well.
    removeFromStack(KEY_HOME);
  }

  if (keyCode == KEY_RIGHT) {
    removeFromStack(KEY_DELETE); // Right arrow released. Remove delete from stack as well.
    removeFromStack(KEY_END);
  }

  if (keyCode == KEY_UP) {
    removeFromStack(KEY_PAGE_UP);
  }

  if (keyCode == KEY_DOWN) {
    removeFromStack(KEY_PAGE_DOWN);
  }

  if (keyCode == MODIFIERKEY_CTRL || keyCode == MODIFIERKEY_SHIFT || keyCode == MODIFIERKEY_ALT) {
    sendKeyStack();
    return;
  }

  removeFromStack(keyCode);
  sendKeyStack();
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
    //keyPress(keyCode);
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
