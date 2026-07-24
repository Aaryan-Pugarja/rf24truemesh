#include "RF24TrueMesh.h"


void RF24truemesh::generateRandomAddress(byte addr[5]) {
  bool unique;

  do {
    unique = true;

    // Generate random address
    for (int i = 0; i < 5; i++) {
      addr[i] = esp_random() & 0xFF;
    }

    // Check against existing addresses
    for (Address &a : usedAddresses) {
      if (memcmp(addr, a.value, 5) == 0) {
        unique = false;
        break;
      }
    }

  } while (!unique);
}




bool RF24truemesh::discovery_mode_parent(DataPacket_parent &data_p, DataPacket_child &data_c) {

  //data_p.who = 1;
  data_p.mode = 0;
  data_p.counter = 0;
  //std::vector<Address> v;
  //data_p.self_id = default_root_address;
  memcpy(data_p.self_id, default_root_address.value, 5);
  //unsigned long previousMillis = 0;
  unsigned long startMillis = 0;

  int lengthmax = 0;
  bool isUnique = true;
  Address addr2;

  radio.openReadingPipe(1, default_broadcast_address.value);
  radio.openWritingPipe(default_broadcast_address.value);

  while (data_p.counter <= 2) {
    temp.clear();
    radio.startWrite(&data_p, sizeof(data_p), true); //Sends broadcast message
    radio.txStandBy(); //Prevents message from not getting sent
    startMillis = millis();
    radio.startListening(); //Starts listening(half duplex)

    while (millis() - startMillis <= 100) {  // causes busy wait and uses 100% of core, will have to use vTaskDelay(pdMS_TO_TICKS(1)) to have 1ms or less free time per loop
      while (radio.available()) { //could be if(radio.availble()), idk yet
        radio.read(&data_c, sizeof(data_c));
        if (data_c.correspond_counter != data_p.counter) { //If true, means a node is lagging significantly
          return false;
        }

        isUnique = true;

        for (Address &addr : temp) {
          if ((memcmp(addr.value, data_c.random_id, 5) == 0)) {
            isUnique = false;
            radio.flush_rx();
            break;
          }
        }
        if (isUnique) {
          memcpy(addr2.value, data_c.random_id, 5);
          temp.push_back(addr2);
        } else {
          radio.flush_rx();
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }
    radio.stopListening(); //Stops listening(can write now)
    if (isUnique) {
      data_p.index = data_p.counter;
    }
    if (data_p.counter == 0) {
      lengthmax = temp.size();
    } else {
      if(lengthmax != temp.size()) {
        
        return false;
      }
    }
    data_p.counter++;
  }
  return true;
}

bool RF24truemesh::assignment_mode_parent(DataPacket_parent &data_p, DataPacket_child &data_c) {
  data_p.mode = 1;
  memcpy(data_p.self_id, default_root_address.value, 5);
  //unsigned long startMillis = 0;
  unsigned long startMillis = 0;
  //std::vector<Address> v2;
  bool heard = false;
  bool node_access = true;
  uint8_t local_counter = 0;
  
  //First assignment message
  radio.stopListening(); //Stops listening(half duplex)
  radio.openReadingPipe(1,default_broadcast_address.value);
  radio.openWritingPipe(default_broadcast_address.value);
  radio.startWrite(&data_p, sizeof(data_p), true); //Sends ID assignment message
  radio.txStandBy(); //Prevents message from not getting sent
  
  for(Address &addr : temp){
    local_counter = 0;
    if(!node_access){
      return false;
    }
    data_p.unique = 1;
    
    //Unique address check(global check)
    for(Address &addr2 : usedAddresses){
      if((memcmp(addr.value, addr2.value, 5) == 0)){
        generateRandomAddress(data_p.supply_id); // WARNING: Does not ensure unique address in temp
        data_p.unique = 0;
        radio.flush_rx();
        break;
      }
    }
    
    while(local_counter<=2){
    heard = false;
      //radio.closeReadingPipe(1); //Perhaps redundant as overwriting pipe address
      radio.openReadingPipe(1,addr.value);
      radio.openWritingPipe(addr.value);
      radio.stopListening();
      radio.startWrite(&data_p, sizeof(data_p), true); //Individual message for checking assignment
      radio.txStandBy(); //Prevents message from not getting sent
      
      radio.startListening();
      
      startMillis = millis();
      while (millis() - startMillis <= 15) {  // causes busy wait and uses 100% of core, will have to use vTaskDelay(pdMS_TO_TICKS(1)) to have 1ms or less free time per loop if no messages available to read
        if (radio.available()) { // skeptical about this, should parent send reply to reply?, should it be while(radio.available) instead and child sends multiple messages to avoid root missing message
          radio.read(&data_c, sizeof(data_c));
          heard = true;
          if(data_p.unique){
            usedAddresses.push_back(addr);
          }else{
            usedAddresses.push_back(Address(data_p.supply_id));
          }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      
      if(heard){
        node_access = true;
        radio.flush_rx();
        break; 
      }else{
        radio.stopListening(); //Stops listening(half duplex)
        radio.openReadingPipe(1, default_broadcast_address.value);
        radio.openWritingPipe(default_broadcast_address.value);
        radio.startWrite(&data_p, sizeof(data_p), true); //Sends ID assignment message
        radio.txStandBy(); //Prevents message from not getting sent
        local_counter++;
        node_access = false;
      }
    }
  }
  return true;
}


bool RF24truemesh::discovery_mode_child(DataPacket_parent &data_p, DataPacket_child &data_c) {

  //data_p.who = 1;
  data_c.mode = 0;
  data_c.correspond_counter = 0;
  bool unassigned = 0;
  uint8_t last_counter = 0;
  //data_p.self_id = default_root_address;
  //memcpy(data_c.self_id, default_root_address, 5);
  //unsigned long previousMillis = 0;

  Address random_list[3];

  radio.openReadingPipe(1, default_root_address.value);
  radio.openWritingPipe(default_root_address.value);
  radio.startListening();
  unsigned long startMillis = millis();

  while (data_p.mode==0) {
    while (radio.available()) {
      radio.read(&data_p, sizeof(data_p));
      radio.stopListening();
      if (data_p.mode == 0) {
        uint32_t waitTime = esp_random() % 61;                   //random wait time between 0 - 60ms
        vTaskDelay(pdMS_TO_TICKS(waitTime));                     //waits(non blocking)
        generateRandomAddress(data_c.random_id);                 //generates random address
        if(data_p.counter!=data_c.correspond_counter+1){
          return false;
        }
        data_c.correspond_counter = data_p.counter;              //updates corresponding counter
        radio.startWrite(&data_c, sizeof(data_c), true);         //Sends broadcast message reply(random address)
        radio.txStandBy();                                       //Prevents message from not getting sent
        memcpy(random_list[data_p.counter].value, data_c.random_id, 5); //Added random message to list
        radio.startListening();
      } else {
        return false;
      }
    }
  }
  
  
  uint8_t pipe;
  bool assigned = false;
  radio.startListening();
  memcpy(final_address.value, random_list[data_p.index].value, 5);
  radio.openReadingPipe(1, final_address.value);
  startMillis = millis();
  while(millis() - startMillis<4000 && !assigned){
    if(radio.available()){
      radio.read(&data_p, sizeof(data_p));
      assigned = true;
      if(data_p.unique == 0){
        memcpy(final_address.value, data_p.supply_id, 5);  // Change default_root_address and update when receiving assignment message
      }
      radio.stopListening();
      radio.startWrite(&data_c, sizeof(data_c), true);
      radio.txStandBy();
    }
  }
  return(assigned);
}