#ifndef ROOMHOLDER_HPP
#define ROOMHOLDER_HPP

#include "Holder.hpp"
#include "Room.h"

class RoomHolder : public Holder<std::unique_ptr<Room>> {
   public:
    void join(const NP_JoinRoom&);
    bool alreadyInside(const Room&, const Client&);
    void addRoom(const std::string& name, short, uint16_t mode, uint8_t delay);
    void addTempRoom(uint16_t mode, Node* game = nullptr, Tournament* _tournament = nullptr);
    void removeIdleRooms();
    void sendRoomList(Client&);
}

#endif