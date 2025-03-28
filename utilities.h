#ifndef Utilities_h
#define Utilities_h

#include <Arduino.h>
#include <Print.h>

extern void ex_print_number_to_buf(char* _buf, float val, int len, int dec = 0, int base = DEC, bool unsgn = false );
extern void ex_print_number_to_stream(Print* stream, float val, int len, int dec = 0);
extern void ex_printf_to_stream(Print* stream, const char* fmt, ...);
extern void ex_print_binary_to_stream(Print* stream,  uint8_t val); 
extern void ex_print_str_to_stream(Print* stream, const char* str, bool pgm = false, int fix_len = 0);
extern float ex_parse_float(char* input_buf, int startpos, int len);

#endif
