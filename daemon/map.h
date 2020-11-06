#if LOG_TRACE >= LOG_LEVEL_MIN
  #define log_pdu_trace(pdu, fn) log_log_pdu(LOG_TRACE, pdu, fn)
#else
  #define log_pdu_trace(pdu, fn)
#endif

#if LOG_DEBUG >= LOG_LEVEL_MIN
  #define log_halmap_debug(root, fn) log_log_halmap(LOG_DEBUG, root, fn)
#else
  #define log_halmap_debug(root, fn)
#endif

halmap *halmap_find(pdu *, halmap *);
void log_log_pdu(int level, pdu *pdu, const char *fn);
void log_log_halmap(int level, halmap *map_root, const char *fn);
