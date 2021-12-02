#include "message.h"
#include <arpa/inet.h>


message::message(uint32_t mux, uint32_t sec, uint32_t typ, std::vector<uint8_t> payload)
: mux(mux), sec(sec), typ(typ), payload(payload) {}

message::message(zmqpp::message &received)
{
    message::sdh_ha_v1 *packet = (message::sdh_ha_v1 *)received.raw_data();
    mux = ntohl(packet->tag.mux); 
    sec = ntohl(packet->tag.sec); 
    typ = ntohl(packet->tag.typ); 
    std::vector<uint8_t> payload(packet->data, packet->data + ntohl(packet->data_len));
    this->payload = payload;
}

void message::send(zmqpp::socket& socket) 
{
    message::sdh_ha_v1 *packet = new message::sdh_ha_v1();
    packet->tag.mux = htonl(mux);
    packet->tag.sec = htonl(sec);
    packet->tag.typ = htonl(typ);
    packet->data_len = htonl(payload.size());
    std::copy(payload.begin(), payload.end(), packet->data);
    socket.send_raw((const char*) packet, sizeof(gaps_tag) + sizeof(packet->data_len) + payload.size(), 0);
    delete packet;
}

message::~message()
{
}
