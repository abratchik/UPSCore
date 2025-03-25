#ifndef Utilities_h
#define Utilities_h

#include <Arduino.h>
#include <HardwareSerial.h>

extern void ex_print_number_to_buf(char* _buf, float val, int len, int dec = 0, int base = DEC, bool unsgn = false );
extern void ex_print_number_to_stream(HardwareSerial* stream, float val, int len, int dec = 0);
extern void ex_printf_to_stream(HardwareSerial* stream, const char* fmt, ...);
extern void ex_print_binary_to_stream(HardwareSerial* stream,  uint8_t val); 
extern void ex_print_str_to_stream(HardwareSerial* stream, const char* str, int len);
extern float ex_parse_float(char* input_buf, int startpos, int len);

#endif
