#include <Arduino.h>
#include "ftostrf.h"

String float_to_string(float value, unsigned char decimalPlaces)
{
  char buf[33];
  return String(ftostrf(value, (decimalPlaces + 2), decimalPlaces, buf));
}

