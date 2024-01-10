#pragma once

#include "Arduino.h"

void wifi_connect(byte mode, unsigned long connection_interval = 0);
void wifi_check_connection();