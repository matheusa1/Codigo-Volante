#define ATUA_CW PB4
#define ATUA_CCW PB5 // HIGH aqui gira CCW
#define ATUA_STRENGTH PB3
 
#define ROTARY_ENC_A 6
#define ROTARY_ENC_B 7
#define ROTARY_ENC_PCINT_A PCINT22
#define ROTARY_ENC_PCINT_B PCINT23
#define ROTARY_ENC_PCINT_AB_IE PCIE2
 
#define POS_SENSOR PB2 // switch (absolute position)
#define GP_BUTTON PB1 // general purpose button
#define GP_BUTTON_GND PB0
 
#include "Rotary.h"
#include <EEPROM.h>
volatile long count = 0; // encoder_rotativo = posicao relativa depois de ligado
volatile bool absolute_sw = false; // chave de posicao do volante ativa?
 
Rotary r = Rotary(ROTARY_ENC_A, ROTARY_ENC_B);
 
bool direction = false;

void initPWM() {
  OCR2A = 0;
  TCCR2A = (1<<WGM20);
  TCCR2B = (1<<CS21);
  TCCR2A |= (1<<COM2A1);
}
 
void setPWM(char val) {
  OCR2A = val;
}
 
void Stop() {
  PORTB |= (1<<ATUA_CW);
  PORTB |= (1<<ATUA_CCW);
}
 
void Idle() {
  setPWM(0);
  PORTB &= ~(1<<ATUA_CW);
  PORTB &= ~(1<<ATUA_CCW);
}
 
void Move(char power, bool cw = true) {
  if (power == 0)
    Idle();
  else {
    if (cw) {
      PORTB |= (1<<ATUA_CW);
      PORTB &= ~(1<<ATUA_CCW);
    } else {
      PORTB &= ~(1<<ATUA_CW);
      PORTB |= (1<<ATUA_CCW);
    }
    setPWM(power);
  }
}

int janela = 5;

int flag = 1;
 
void findMiddleOnReset() {
  while(!absolute_sw) {
    Move(170, true);
  }
  while(absolute_sw) {
    Move(170, true);
  }
  int lastCount = count;
  // _delay_ms(1);
  int value;
  if(flag != 1) EEPROM.put(0, (3600 - lastCount));
  EEPROM.get(0, value);
  count = 0;
  while(!(count > value && count < value  + janela)) {
    Move(165, true);
  }
  Stop();
  count = 0;
}

int janela2 = 5;
 
void findAbsolutePosition() {
  if (count > (0 - janela2) && count < janela2) {
    Stop();
  } else {
    count >= 0 ? direction = false : direction = true;
    Move(165, direction);
  }
}

unsigned long tempo = millis();
 

void setup() {
  Serial.begin(115200);
  r.begin(true);
  PCICR |= (1 << ROTARY_ENC_PCINT_AB_IE);
  PCMSK2 |= (1 << ROTARY_ENC_PCINT_A) | (1 << ROTARY_ENC_PCINT_B);
 
  DDRB |= (1<<GP_BUTTON_GND);
  DDRB &= ~((1<<GP_BUTTON)|(1<<POS_SENSOR));
  DDRB |= (1<<ATUA_CW)|(1<<ATUA_CCW)|(1<<ATUA_STRENGTH);
 
  PORTB |= (1<<POS_SENSOR);
  PORTB &= ~(1<<GP_BUTTON_GND);
  PORTB |= (1<<GP_BUTTON);
  PORTB &= ~(1<<ATUA_CW);
  PORTB &= ~(1<<ATUA_CCW);
 
  // EEPROM.put(0, 569);

  initPWM();
  sei();


  findMiddleOnReset();

  tempo = millis();
  while(millis() - tempo < 2000) findAbsolutePosition();
  flag = 0;

  //Idle();
 }
 
int dir = ATUA_CCW;

unsigned long btn_clicked = millis();
char laststate = 0;

void loop() {
  Stop();
  // debug only info
  if (millis()%200==0) {
    Serial.print(count);
    Serial.print(", ");
    Serial.println(absolute_sw==true?'1':'0');
  }
  
  char leitura = PINB & (1<<GP_BUTTON);
  if(leitura != laststate && (millis()- btn_clicked)>10){
    if(leitura == 0) {
      count = 0;
      EEPROM.put(0, count);
      findMiddleOnReset();
      tempo = millis();
      while(millis() - tempo < 2000) findAbsolutePosition();
    }

    btn_clicked = millis();
    laststate = leitura;
  }
  
 }
 
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_NONE) {
    // do nothing
  }
  else if (result == DIR_CW) {
    count--;
  }
  else if (result == DIR_CCW) {
    count++;
  }
 
  absolute_sw = 0==(PINB&(1<<POS_SENSOR));
}
