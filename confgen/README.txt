This directory contain the script (hal_autoconfig.py) to auto-generate HAL 
configuration. The script is called in the CLOSURE toolchain by Makefile.mbig. 
The tool requires two json formatted inputs:

  1) Maps file: The GEDL tool, RPCGenerator.py, generates a json map file
     (when called in Makefile.gedl it is named xdconf.ini). It contains all  
     the information needed for the HAL maps, consistent with the  
     auto-generated application cross-domain RPCs. Input examples from this
     directory include:
	6month demo app:   xdconf_6modemo.json
        ERI demo example:  xdconf_eri.json

  2) Devices file: This file must be manually created, consistent with the 
     node devices in the scenario. In the CLOSURE toolchain, Makefile.mbig 
     looks for this file in ~/gaps/build/src/emu/config/$(PROG)/devices.json, 
     where PROG is the name of the application (e.g., example1). Input 
     examples from this directory include:
        6month demo ILIP loopback (BE):  devices_6modemo_ilip.json
        6month demo IP loopback (BW):    devices_6modemo_jaga_bw.json
        Emulator serial devices:         devices_eri_be.json, 
        ERI IP loopback (BW):            devices_eri_bw.json, 
        ERI ILIP loopback (BE):          devices_eri_be_different_dev_paths.json).

The start of the script describes how these example inputs can be passed as
input options.  The code uses the json python library to parse the inputs and
the libconf reader/writer to output the HAL configuration files in the 
libconfig format used by the HAL daemon.  The libconf library can be installed using:

   pip3 install libconf
