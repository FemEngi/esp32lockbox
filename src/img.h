#ifndef IMG_H
#define IMG_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

void imgupload(const char* filename);

void pinChangeInterrupt();
void checkupdate();

extern const char pin1;
extern const char pin2;


#endif // IMG_H