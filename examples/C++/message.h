#include <cstdint>
#include <vector>
#include <zmqpp/zmqpp.hpp>
#define ADU_SIZE_MAX_C  1000000     /* 1 MB */

class message
{
private:
    struct gaps_tag 
    {
        uint32_t    mux;      /* APP ID */
        uint32_t    sec;      /* Security tag */
        uint32_t    typ;      /* data type */
    };

    // CLOSURE packet
    struct sdh_ha_v1
    {
        gaps_tag  tag;
        uint32_t  data_len;               /* 0 = no immediate data */
        uint8_t   data[ADU_SIZE_MAX_C];   /* Immediate data */
    };

public:
    uint32_t mux;
    uint32_t sec;
    uint32_t typ;
    std::vector<uint8_t> payload;
    message(uint32_t mux, uint32_t sec, uint32_t typ, std::vector<uint8_t> payload);
    message(zmqpp::message &received);
    void send(zmqpp::socket& socket);
    ~message();
};

