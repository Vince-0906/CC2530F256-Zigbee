#ifndef KEY_H
#define KEY_H

#define KEY_EVENT_NONE 0
#define KEY_EVENT_SW1  1
#define KEY_EVENT_SW2  2

void Key_Init(void);
void Key_Process1ms(void);
unsigned char Key_GetEvent(void);

#endif
