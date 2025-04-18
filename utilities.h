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

const PROGMEM int SINEX10000[] = {
    0, 175, 349, 523, 698, 872, 1045, 1219, 1392, 1564, 1736, 1908, 2079, 2249, 2419, 2588,
    2756, 2924, 3090, 3256, 3420, 3584, 3746, 3907, 4067, 4226, 4384, 4540, 4695, 4848, 5000,
    5150, 5299, 5446, 5592, 5736, 5878, 6018, 6157, 6293, 6428, 6560, 6691, 6820, 6946, 7071,
    7193, 7313, 7431, 7547, 7660, 7771, 7880, 7986, 8090, 8191, 8290, 8387, 8480, 8572, 8660,
    8746, 8829, 8910, 8988, 9063, 9135, 9205, 9272, 9336, 9397, 9455, 9511, 9563, 9613, 9659,
    9703, 9744, 9781, 9816, 9848, 9877, 9903, 9925, 9945, 9962, 9976, 9986, 9994, 9998, 10000
};

extern float ex_fast_sine(int angle);

#endif
