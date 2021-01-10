Auto-generaion of HAL Config Files in the CLOSURE Toolchain
January 2021

This directory contain the python3 script (hal_autoconfig.py) to auto-generate 
HAL configuration. The script is called in the CLOSURE toolchain by 
Makefile.mbig. The tool requires two json formatted inputs:

  1) Maps file (in directory map_defs): 
     The CLOSURE GEDL tool, RPCGenerator.py, generates a json map file
     (when called in Makefile.gedl it is named xdconf.ini). It contains all  
     the information needed for the HAL maps, consistent with the  
     auto-generated application cross-domain RPCs. Input examples from this
     directory include:
	6month demo app:   xdconf_6modemo.json
        ERI demo example:  xdconf_eri.json

  2) Devices file (in directory devices_defs)
     File is manually created, consistent with the devices in the scenario
     In the CLOSURE toolchain, Makefile.mbig looks for this file in:
       ~/gaps/build/src/emu/config/$(PROG)/devices.json, 
     where PROG is the name of the application (e.g., example1). Input 
     examples from this directory include:
        Emulator serial devices:       devices_eop_socat.json
        6month demo ILIP (BE):         devices_6modemo_ilip.json
        ERI IP loopback (BW):          devices_eri_jaga_bw.json, 
        6month IP MIND demo (direct):  devices_6modemo_mind_bw.json

The start of the script describes how these example inputs can be passed as
input options.  The code uses the json python library to parse the inputs and
the libconf reader/writer to output the HAL configuration files in the 
libconfig format used by the HAL daemon.  The libconf library can be installed using:

   pip3 install libconf
