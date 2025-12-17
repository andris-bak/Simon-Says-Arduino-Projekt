  #include <LiquidCrystal.h>
  #include <EEPROM.h>    

  const int EEPROM_ADDR_HS = 0;      

  LiquidCrystal lcd(11, 12, A0, A1, A2, A3); 

  const int NUM_BUTTONS = 4;

  int ledPins[NUM_BUTTONS]    = {2, 3, 4, 5};
  int buttonPins[NUM_BUTTONS] = {6, 7, 8, 9};
  int buzzerPin = 10;

  const int MAX_STEPS = 50;
  int sequenceSteps[MAX_STEPS];

  int level = 0;        
  int userIndex = 0;   
  int score = 0;        

  int sessionHigh = 0;  
  int allTimeHigh = 0;  

  bool newRound = true;

  int baseDelay = 400;
  int minDelay  = 120;

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

  void updateLcd() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Level:");
    lcd.print(level);

    lcd.setCursor(0, 1);
    lcd.print("SCORE:");
    lcd.print(score);
    lcd.print(" HS:");
    lcd.print(sessionHigh);   
  }

  // játék reset
  void resetGame() {
    level = 0;
    userIndex = 0;
    score = 0;
    newRound = true;        
    updateLcd();
  }

  void setup() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      pinMode(ledPins[i], OUTPUT);
      digitalWrite(ledPins[i], LOW);
      pinMode(buttonPins[i], INPUT_PULLUP);
    }
    pinMode(buzzerPin, OUTPUT);

    lcd.begin(16, 2);
    EEPROM.get(EEPROM_ADDR_HS, allTimeHigh);

    if (allTimeHigh < 0 || allTimeHigh > 3000) {
      allTimeHigh = 0;
    } 

    sessionHigh = allTimeHigh;

    for (int i = 0; i < NUM_BUTTONS; i++) {
      digitalWrite(ledPins[i], HIGH);
      beep(500 + i * 150, 130);
      digitalWrite(ledPins[i], LOW);
      delay(80);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SIMON SAYS");
    lcd.setCursor(0, 1);
    lcd.print("Press any key");

    unsigned long seed = 0;
    while (readButton() == -1) {   
      seed++;
      delay(1);
    }
    randomSeed(seed);
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
        score++;            

        if (score > sessionHigh) {
          sessionHigh = score;  
        }
        if (score > allTimeHigh) {
          allTimeHigh = score;   
          EEPROM.put(EEPROM_ADDR_HS, allTimeHigh);  
        }
        updateLcd();

        if (userIndex >= level) {
          victoryFlash();
          delay(400);
          newRound = true;
        }
      } else {
        gameOver();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("GAME OVER");
        lcd.setCursor(0, 1);
        lcd.print("SCORE:");
        lcd.print(score);
        lcd.print(" HS:");
        lcd.print(allTimeHigh);   

        delay(2500);
        resetGame();
      }
    }
    delay(5);
  }
