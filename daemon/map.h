#if LOG_TRACE >= LOG_LEVEL_MIN
  #define log_pdu_trace(pdu, fn) log_log_pdu(level, pdu, fn)
#else
  #define log_pdu_trace(pdu, fn)
#endif

void selector_print(selector *, FILE *);
void halmap_print_one(halmap *);
void halmap_print_all(halmap *, int, const char *);
halmap *halmap_find(pdu *, halmap *);
