#pragma once

#include <Arduino.h>

struct Address
{
    byte value[5];
};

struct DataPacket_parent {
  bool mode;
  bool unique;
  uint8_t counter;
  byte self_id[5];
  byte supply_id[5];
  uint8_t index;

  DataPacket_parent() {
    mode = 0;
    unique = 1;
    counter = 0;
    //memcpy(self_id, default_root_address, 5);
    index = 0;
  }
};

struct DataPacket_child {
  bool mode;
  uint8_t correspond_counter;
  byte random_id[5];

  DataPacket_child() {
    mode = 0;
    correspond_counter = 0;
  }
};
