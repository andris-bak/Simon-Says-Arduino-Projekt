#include <LiquidCrystal.h>

// ----- LCD -----
LiquidCrystal lcd(11, 12, A0, A1, A2, A3); 
// RS=11, E=12, D4=A0, D5=A1, D6=A2, D7=A3

// ----- Simon Says beállítások -----
const int NUM_BUTTONS = 4;

int ledPins[NUM_BUTTONS]    = {2, 3, 4, 5};
int buttonPins[NUM_BUTTONS] = {6, 7, 8, 9};
int buzzerPin = 10;

const int MAX_STEPS = 50;
int sequenceSteps[MAX_STEPS];

int level = 0;        // hány elem van most a sorozatban
int userIndex = 0;    // hányadik elemet ütjük be
int score = 0;        // pontszám (teljesített lépések száma)

bool newRound = true;

// sebesség: minél nagyobb a level, annál kisebbek a késleltetések
int baseDelay = 400;      // kezdő villanási idő (ms)
int minDelay  = 120;      // ennél gyorsabbra már ne menjen

// ----- Segédfüggvények -----

void beep(int freq, int duration) {
  tone(buzzerPin, freq, duration);
  delay(duration);
  noTone(buzzerPin);
}

// egy LED felvillantása + hang
void flashLed(int index, int duration) {
  digitalWrite(ledPins[index], HIGH);
  beep(400 + index * 120, duration); 
  digitalWrite(ledPins[index], LOW);
  delay(80);
}

// a teljes sorozat lejátszása a játékosnak
void showSequence(int len) {
  // level alapján gyorsuljon
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

// gomb olvasása: visszaadja a gomb indexét (0–3), vagy -1-et, ha nincs nyomva
int readButton() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(buttonPins[i]) == LOW) { // INPUT_PULLUP -> lenyomva = LOW
      // várjuk meg, míg felengedi (debounce)
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
  // villogtatjuk az összes LED-et párszor + „szomorú” hang
  for (int i = 0; i < 3; i++)
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

// LCD frissítése: szint + pontszám
void updateLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(level);

  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
}

// játék reset
void resetGame() {
  level = 0;
  userIndex = 0;
  score = 0;
  newRound = true;
  updateLcd();
}

// ----- setup és loop -----

void setup() {
  // LED-ek, gombok
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
  
  // random seed
  randomSeed(analogRead(A5));

  // kis induló LED / hang animáció
  for (int i = 0; i < NUM_BUTTONS; i++) {
    digitalWrite(ledPins[i], HIGH);
    beep(500 + i * 150, 130);
    digitalWrite(ledPins[i], LOW);
    delay(80);
  }

  // várjunk az első gombnyomásra, hogy induljon a játék
  while (readButton() == -1) {
    // semmi, csak várakozás
  }

  resetGame();
}

void loop() {
  if (newRound) {
    if (level < MAX_STEPS) {
      sequenceSteps[level] = random(0, NUM_BUTTONS);
      level++;
    }
    updateLcd();        // frissítjük a szintet
    showSequence(level);
    userIndex = 0;
    newRound = false;
  }

  int pressed = readButton();

  if (pressed != -1) {
    // rövid visszajelzés: felvillan a lenyomott LED + hang
    flashLed(pressed, 120);

    // jó gomb?
    if (pressed == sequenceSteps[userIndex]) {
      userIndex++;
      score++;         // minden helyes lépésért +1 pont
      updateLcd();

      // ha az összeset jól visszajátszotta
      if (userIndex >= level) {
        victoryFlash();
        delay(400);
        newRound = true;   // következő szint
      }
    } else {
      // rossz gomb -> game over
      gameOver();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAME OVER");
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(score);

      delay(2000);

      // új játék
      resetGame();
    }
  }

  delay(5);
}