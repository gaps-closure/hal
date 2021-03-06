#define SDH_BE_V3_ADU_SIZE_MAX   176    /* 0x048 – 0x0f7 */

/* GAPS packet format ($ = modified from v1, % = new in v2, ^ = new in v3)) */
typedef struct _pkt_sdh_be_v3 {
    uint32_t  session_tag;              /*   Application Mux TAG */
    uint32_t  message_tag;              /*   Security TAG */
    uint32_t  descriptor_type;          /* $ Packet type (DMA channel) v3=1-3 */
    uint32_t  descriptor_tag;           /* ^ Any, security policy dependent */
  
    uint32_t  gaps_time_lo;             /*   ILIP GAPS Time (set by ILIP) */
    uint32_t  gaps_time_up;             /*   -- uint64_t causes struct padding */
    uint32_t  linux_time_lo;            /*   Time Set by Driver */
    uint32_t  linux_time_up;            /*   (Secs & micro-seconds since 1970) */
  
    uint32_t  imm_data_len;             /*   Immediate Data Length (in bytes) */
    uint32_t  destination_tag;          /* % Multipurpose */
    uint32_t  pkt_sip_hash_0;           /* ^ 128-bit SipHash of 256 byte packet */
    uint32_t  pkt_sip_hash_1;           /* ^ (Four 32-bit words set by ILIP) */
  
    uint32_t  pkt_sip_hash_2;           /* ^ */
    uint32_t  pkt_sip_hash_3;           /* ^ */
    uint32_t  data_tag;                 /*   Type TAG (e.g., DATA_PAYLOAD_1) */
    uint32_t  dma_data_len;             /* ^ DMA Data Length (in bytes) */
  
    uint64_t dma_data_addr_lo;          /* ^ Host Payload Address for DMA */
//    uint32_t  dma_data_addr_lo;         /* ^ Host Payload Address for DMA */
//    uint32_t  dma_data_addr_up;         /* ^ as Two 32-bit words */
    uint8_t   imm_data[SDH_BE_V3_ADU_SIZE_MAX];     /* Optional Immediate Data */
    uint32_t  desc_sip_hash_0;          /* % 64-bit message description SipHash */
    uint32_t  desc_sip_hash_1;          /* % Two 32-bit words are set by ILIP */
} pkt_sdh_be_v3;

/* exported functions */
int  pdu_from_sdh_be_v3 (pdu *, uint8_t *, int);
int  pdu_into_sdh_be_v3 (uint8_t *, pdu *, gaps_tag *);
