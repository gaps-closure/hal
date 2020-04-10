In the actual hardware testbed, you only need the configuration files (network config is doen by system administrator):
  sample_6modemo_bw_green.cfg
  sample_6modemo_bw_orange.cfg

If using HALperf rather than the actual mission application, you also need:
  green-halperf.sh
  orange-halperf.sh

If testing in CORE emulator, you additionally need the CORE scenario, and network configuration files:
  bwtestenv.imn
  green-netconf.sh
  orange-netconf.sh
