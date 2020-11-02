/* Define GAPS Packet Format for SDH BE */

#define SDH_BE_V2_ADU_SIZE_MAX   208    /* 0x028 – 0x0f7 */

/* packet ($ = filed is modified from v1, % = field is new in v2) */
typedef struct _pkt_sdh_be_v2 {
    uint32_t  session_tag;              /*   Application Mux TAG */
    uint32_t  message_tag;              /*   Security TAG */
    uint32_t  descriptor_type;          /* $ Packet type (DMA channel) v3=1-3 */
    uint32_t  data_tag;                 /*   Type TAG (e.g., DATA_PAYLOAD_1) */
    uint32_t  gaps_time;                /*   ILIP GAPS Time (set by ILIP) */
    uint32_t  gaps_time_us;             /*   -- uint64_t causes struct padding */
    uint32_t  linux_time;               /*   Time Used/Set by Application */
    uint32_t  linux_time_us;            /*   Secs & micro-seconds since 1970 */
    uint32_t  destination_tag;          /* % Multipurpose */
    uint32_t  imm_data_len;             /*   Immediate Data Length (in bytes) */
    uint8_t   imm_data[SDH_BE_V2_ADU_SIZE_MAX];     /* Optional Immediate Data */
    uint32_t  desc_sip_hash_0;          /* % 64-bit message description SipHash */
    uint32_t  desc_sip_hash_1;          /* % Two 32-bit words are set by ILIP */
} pkt_sdh_be_v2;

/* exported functions */
void pdu_from_sdh_be_v2 (pdu *, uint8_t *, int);
int  pdu_into_sdh_be_v2 (uint8_t *, pdu *, gaps_tag *);
