#include "parseVal.h"

constexpr int16_t p10 = 10;        // 10 value used to automatically promote
constexpr int16_t p100 = 100;      // 100 value used to automatically promote
constexpr int16_t d0 = '0';        // ASCII value, 0
constexpr int16_t d02d = d0 * 11;  // difference for form 0.0
constexpr int16_t d03d = d0 * 111; // difference for form 00.0

int16_t parseVal(uint8_t *ch)
{
  if (ch[0] == '-')
  {
    if (ch[2] == '.')
    {
      return (ch[1] * p10 + ch[3] - d02d) * -1;
    }
    else
    {
      return (ch[1] * p100 + ch[2] * p10 + ch[4] - d03d) * -1;
    }
  }
  else
  {
    if (ch[1] == '.')
    {
      return ch[0] * p10 + ch[2] - d02d;
    }
    else
    {
      return ch[0] * p100 + ch[1] * p10 + ch[3] - d03d;
    }
  }
}