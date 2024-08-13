#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

void keyboard_init(void);
void intr_keyboard_handler(void);

extern struct ioqueue kbd_buf;

#endif