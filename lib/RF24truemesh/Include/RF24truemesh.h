#pragma once

#include <RF24.h>
#include "Types.h"
#include <vector>

class RF24truemesh
{
public:
    RF24truemesh(
        RF24& radio,
        Address broadcast = {{0x00, 0x00, 0x00, 0x00, 0x01}},
        Address root      = {{0x00, 0x00, 0x00, 0x00, 0x02}})
        : radio(radio),
          default_broadcast_address(broadcast),
          default_root_address(root), usedAddresses({broadcast, root})
    {
    }

    bool discovery_mode_parent(DataPacket_parent &data_p, DataPacket_child &data_c);
    bool assignment_mode_parent(DataPacket_parent &data_p, DataPacket_child &data_c);
    bool discovery_mode_child(DataPacket_parent &data_p, DataPacket_child &data_c);
    
    std::vector<Address> usedAddresses;
    std::vector<Address> reachable;
    std::vector<Address> temp;
    
    private:
    RF24& radio;
    
    Address final_address;
    Address default_broadcast_address;
    Address default_root_address;
    void generateRandomAddress(byte[5]);
    
};