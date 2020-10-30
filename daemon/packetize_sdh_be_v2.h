/* Define GAPS Packet Format for SDH BE */

#define SDH_BE_V2_ADU_SIZE_MAX   208

/* packet ($ = filed is modified from v1, % = field is new in v2) */
typedef struct _pkt_sdh_be_v2 {
    uint32_t  session_tag;              /*   App Mux */
    uint32_t  message_tag;              /*   Security */
    uint32_t  message_descriptor;       /* $ Packet type (DMA channel) v2=0 */
    uint32_t  data_tag;                 /*   Type (e.g., DATA_PAYLOAD_1) */
    uint32_t  gaps_time;                /*   ILIP GAPS Time (set by ILIP) */
    uint32_t  gaps_time_us;             /*   -- uint64_t causes struct padding */
    uint32_t  linux_time;               /*   Time Used/Set by Application */
    uint32_t  linux_time_us;            /*   Secs & micro-seconds since 1970 */
    uint32_t  destination_tag;          /* % Multipurpose */
    uint32_t  data_len;                 /*   Data Length (in bytes) */
    uint8_t   data[SDH_BE_V2_ADU_SIZE_MAX];
    uint32_t  sip_hash_low              /* % 64-bit SipHash (set by ILIP) */
    uint32_t  sip_hash_high             /* % MAC protects message_descriptor */
} pkt_sdh_be_v2;

/* exported functions */
void pdu_from_sdh_be_v2 (pdu *, uint8_t *, int);
int  pdu_into_sdh_be_v2 (uint8_t *, pdu *, gaps_tag *);
