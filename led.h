#ifndef LED_H
#define LED_H

#define LED_ID_1 1
#define LED_ID_2 2
#define LED_ID_3 3
#define LED_ID_4 4

void LED_Init(void);
void LED_On(unsigned char led_id);
void LED_Off(unsigned char led_id);
void LED_AllOff(void);
void LED_ShowSingle(unsigned char led_id);
void LED_Toggle(unsigned char led_id);

#endif
