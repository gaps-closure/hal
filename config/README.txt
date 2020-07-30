This directory contain the script (hal_autoconfig.py) to auto-generate HAL 
configuration. The script is called in CLOSURE toolchain by Makefile.mbig. 
The tool requires two json formatted inputs:

  1) Maps file: The GEDL tool, RPCGenerator.py, generates a json map file
     (when called in Makefile.gedl it is named xdconf.ini). It contains all  
     the information needed for the HAL maps, consistent with the  
     auto-generated application cross-domain RPCs.

  2) Devices file: This file must be manually created, consistent with the 
     node devices in the scenario. In the CLOSURE toolchain, Makefile.mbig 
     looks for this file in ~/gaps/build/src/emu/config/$(PROG)/devices.json, 
     where PROG is the name of the application (e.g., example1).

In addition to the python HAL configuration script, this directory also has 
example of the two inputs for testing. These include:

  1) Maps file: based on the 6month demo application (xdconf_6modemo.json)
     and the ERI examples (xdconf_eri.json).

  2) Devices file: based on the 6month demo serial loopback devices 
     (devices_6modemo_ilip.json) and the bump-in-the-wire IP devices
     (devices_6modemo_jaga_bw.json). It also has examples of the serial
     devices used in the emulator (devices_eri_be.json, devices_eri_bw.json, 
     devices_eri_be_different_dev_paths.json).

The start of the script describes how these example inputs can be passed as
input options.  The code uses the json python library to parse the inputs 
and the libconf reader/writer to output the HAL configuration files in the 
libconfig format used by the HAL daemon.  If not found, the libconf library 
can be installed using:

   pip3 install libconf
