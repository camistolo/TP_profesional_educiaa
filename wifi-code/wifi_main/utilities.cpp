#include "utilities.h"

//////////////////////////////////////////////////////////////////////////////////////

void serial_print (char meas_str)
{ 
  Serial.print(">");
  Serial.print(meas_str);
  Serial.print("<");
}
