#include <LiquidCrystal.h>
#include <EEPROM.h>                 // ÚJ: all-time high score miatt

// ----- EEPROM cím a high score-nak -----
const int EEPROM_ADDR_HS = 0;       // ide mentjük az all-time high score-t

// ----- LCD -----
LiquidCrystal lcd(11, 12, A0, A1, A2, A3); 

// ----- Simon Says beállítások -----
const int NUM_BUTTONS = 4;

int ledPins[NUM_BUTTONS]    = {2, 3, 4, 5};
int buttonPins[NUM_BUTTONS] = {6, 7, 8, 9};
int buzzerPin = 10;

const int MAX_STEPS = 50;
int sequenceSteps[MAX_STEPS];

int level = 0;        // hány elem van most a sorozatban
int userIndex = 0;    // hányadik elemet ütjük be
int score = 0;        // aktuális pontszám (jelen játék)

int sessionHigh = 0;  // ÚJ: aktuális futás legjobb pontja
int allTimeHigh = 0;  // ÚJ: EEPROM-ból töltött „örök” high score

bool newRound = true;

// sebesség: minél nagyobb a level, annál gyorsabb
int baseDelay = 400;
int minDelay  = 120;

// ----- Segédfüggvények -----

void beep(int freq, int duration) {
  tone(buzzerPin, freq, duration);
  delay(duration);
  noTone(buzzerPin);
}

void flashLed(int index, int duration) {
  digitalWrite(ledPins[index], HIGH);
  beep(400 + index * 120, duration); 
  digitalWrite(ledPins[index], LOW);
  delay(80);
}

void showSequence(int len) {
  int currentDelay = baseDelay - level * 15;
  if (currentDelay < minDelay) currentDelay = minDelay;

  delay(400);
  for (int i = 0; i < len; i++) {
    int idx = sequenceSteps[i];
    digitalWrite(ledPins[idx], HIGH);
    tone(buzzerPin, 400 + idx * 120);
    delay(currentDelay);
    noTone(buzzerPin);
    digitalWrite(ledPins[idx], LOW);
    delay(120);
  }
}

int readButton() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(buttonPins[i]) == LOW) { 
      while (digitalRead(buttonPins[i]) == LOW) {
        delay(1);
      }
      delay(40);
      return i;
    }
  }
  return -1;
}

void gameOver() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < NUM_BUTTONS; j++) {
      digitalWrite(ledPins[j], HIGH);
    }
    beep(220, 200);
    for (int j = 0; j < NUM_BUTTONS; j++) {
      digitalWrite(ledPins[j], LOW);
    }
    delay(200);
  }
}

// kis örömanimáció, ha teljesített egy szintet
void victoryFlash() {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < NUM_BUTTONS; j++) {
      digitalWrite(ledPins[j], HIGH);
    }
    beep(900, 100);
    for (int j = 0; j < NUM_BUTTONS; j++) {
      digitalWrite(ledPins[j], LOW);
    }
    delay(100);
  }
}

// ----- LCD frissítés: level, score, session high -----
void updateLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lvl:");
  lcd.print(level);

  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(score);
  lcd.print(" HS:");
  lcd.print(sessionHigh);   // ÚJ: mindig mutatjuk a futas legjobb pontjat
}

// játék reset
void resetGame() {
  level = 0;
  userIndex = 0;
  score = 0;
  // sessionHigh és allTimeHigh NEM nullázódik
  newRound = true;        // <<< EZ HIÁNYZOTT
  updateLcd();
}

// ----- setup és loop -----

void setup() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(buzzerPin, OUTPUT);

  // LCD
  lcd.begin(16, 2);
  lcd.print("SIMON SAYS");
  lcd.setCursor(0, 1);
  lcd.print("Press any key");

  // EEPROM-ból betöltjük az all-time high score-t
  allTimeHigh = EEPROM.read(EEPROM_ADDR_HS);   // 0-255-ig bőven elég
  sessionHigh = allTimeHigh;                   // induláskor a kettő ugyanaz

  // random seed
  //randomSeed(analogRead(A5));
  

  // kis induló animáció
  for (int i = 0; i < NUM_BUTTONS; i++) {
    digitalWrite(ledPins[i], HIGH);
    beep(500 + i * 150, 130);
    digitalWrite(ledPins[i], LOW);
    delay(80);
  }

  // várunk egy gombnyomásra
  while (readButton() == -1) {
    // csak vár
  }
  
  // ------------------------------------------------
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press any key");

  unsigned long seed = 0;
  while (readButton() == -1) {
    seed++;
    delay(1);
  }
  
  randomSeed(seed);
  // --------------------------------------------------

  resetGame();
}

void loop() {
  if (newRound) {
    if (level < MAX_STEPS) {
      sequenceSteps[level] = random(0, NUM_BUTTONS);
      level++;
    }
    updateLcd();
    showSequence(level);
    userIndex = 0;
    newRound = false;
  }

  int pressed = readButton();

  if (pressed != -1) {
    flashLed(pressed, 120);

    if (pressed == sequenceSteps[userIndex]) {
      userIndex++;
      score++;                 // aktuális pont

      // --- HIGH SCORE FRISSÍTÉS ---  // ÚJ rész
      if (score > sessionHigh) {
        sessionHigh = score;   // futás közbeni legjobb
      }
      if (score > allTimeHigh) {
        allTimeHigh = score;   // örök legjobb
        EEPROM.write(EEPROM_ADDR_HS, allTimeHigh);  // elmentjük
      }
      // ------------------------------

      updateLcd();

      if (userIndex >= level) {
        victoryFlash();
        delay(400);
        newRound = true;
      }
    } else {
      // rossz gomb -> game over
      gameOver();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAME OVER");
      lcd.setCursor(0, 1);
      lcd.print("S:");
      lcd.print(score);
      lcd.print(" HS:");
      lcd.print(allTimeHigh);   // itt az all-time high-t mutatjuk

      delay(2500);

      resetGame();
    }
  }

  delay(5);
}
