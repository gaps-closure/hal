#!/usr/bin/env python3

# Create HAL configuration file
#    March, 2021 (added support for ZMQ API)
#
# Tested Examples:
#  1) eri demo running in emulator (start.sh example1) with /dev/vcom devices (default inputs)
#       python3 hal_autoconfig.py
#  2) 6month demo running both enclaves on jaga (with sudo bash test/6MoDemo_BW.net.sh)
#       python3 hal_autoconfig.py -d device_defs/devices_6modemo_jaga_bw.json
#  3) 6month demo running both enclaves on jaga (with ilip root, read and write devices)
#       python3 hal_autoconfig.py -d device_defs/devices_6modemo_ilip_v3.json
# Untested Examples:
#  4) eri demo running both enclaves on jaga with BW devices (but not send to gw listners)
#       python3 hal_autoconfig.py -x xdc_defs/xdconf_eri.json -d xdc_defs/devices_eri_bw.json
#  5) eri demo running in emulator (start.sh example1) with different BE device paths
#       python3 hal_autoconfig.py -d  device_defs/devices_be_different_dev_paths.json

import json
import argparse
import libconf
import sys

# Money patching to avoid dumping L suffix for integers > 32-bit signed
def is_long_int(i): return False
libconf.is_long_int = is_long_int
        
def get_args():
    parser = argparse.ArgumentParser(description='Create HAL configuration file')
    parser.add_argument('-d', '--json_devices_file',  help='Input JSON file name of HAL device conig', type=str, default='device_defs/devices_socat.json')
    parser.add_argument('-o', '--output_dir', help='Output directory path', type=str, default='')
    parser.add_argument('-p', '--output_file_prefix', help='Output HAL configuration file name prefix', type=str, default='hal')
    parser.add_argument('-v', '--verbose', help="run in verbose mode", action='store_true', default=False)
    parser.add_argument('-x', '--json_api_file',      help='Input JSON file name of HAL API and tag-maps', type=str, default='xdc_defs/xdconf_6modemo.json')
    return parser.parse_args()

# Read JSON file into a python dictionary (data)
def read_json(file_name):
    try:
        with open(file_name, 'r') as json_file:
            try:
                data = json.load(json_file)
            except json.JSONDecodeError as err:
                print (file_name, 'file has json error:\n  ', err)
                sys.exit()
    except FileNotFoundError:
        print (file_name, 'file does not exist')
        sys.exit()
    if (args.verbose):
        print ('Read json file', file_name)
        print (json.dumps(data, indent=2))
    return (data)

# Write python dictionary to HAL configuration file
def write_hal_config_file(file_name, dict):
    with open(file_name, 'w') as f:
        print ('Writing HAL configuration into file:' , file_name)
        libconf.dump(dict, f)

###############################################################
# Put Config File device and map information into dictionaries
###############################################################
# Add ipc speciffic information to mutable dictionaty (d)
def add_ipc(local_enclve_name, d, dev, enc_info):
    d['mode_in']   = dev['mode_in']
    d['mode_out']  = dev['mode_out']
    d['addr_in']   = enc_info['outuri']
    d['addr_out']  = enc_info['inuri']

# Add tty speciffic information to mutable dictionaty (d)
def add_tty(local_enclve_name, d, dev):
    if 'path_1' in dev:         # not using same name for both envlaves in dev['path']
        if (local_enclve_name == dev['enclave_name_1']):
            d['path']  = dev['path_1']
        else:
            d['path']  = dev['path_2']


# Add ipc speciffic infofrmation (address/port) to mutable dictionaty (d)
#   connect_addr_1 is found when sending to xd gaurd (not directly to the other enclave)
def add_inet(local_enclve_name, d, dev):
    if (local_enclve_name == dev['enclave_name_1']):
        d['addr_in']    = dev['listen_addr_1']
        d['port_in']    = dev['listen_port_1']
        if 'connect_addr_1' in dev:
            d['addr_out']   = dev['connect_addr_1']
            d['port_out']   = dev['connect_port_1']
        else:
            d['addr_out']   = dev['listen_addr_2']
            d['port_out']   = dev['listen_port_2']
    else:
        d['addr_in']    = dev['listen_addr_2']
        d['port_in']    = dev['listen_port_2']
        if 'connect_addr_1' in dev:
            d['addr_out']   = dev['connect_addr_2']
            d['port_out']   = dev['connect_port_2']
        else:
            d['addr_out']   = dev['listen_addr_1']
            d['port_out']   = dev['listen_port_1']
            
# Add serial device info to mutable dictionaty (d)
def add_ilp(local_enclve_name, d, dev):
    if 'enclave_name_1' in dev:
        if (local_enclve_name == dev['enclave_name_1']):
            d['path_r']      = dev['path_r_1']
            d['path_w']      = dev['path_w_1']
            d['from_mux']    = dev['from_mux_1']
            d['init_enable'] = dev['init_at_1']
    if 'enclave_name_2' in dev:
        if (local_enclve_name == dev['enclave_name_2']):
            d['path_r']      = dev['path_r_2']
            d['path_w']      = dev['path_w_2']
            d['from_mux']    = dev['from_mux_2']
            d['init_enable'] = dev['init_at_2']
            
# Create HAL device configurations as a list of python dictionaries
def create_device_cfg(dev_dict, enc_info, local_enclve_name):
    index=0
    hal_config_dev_list = []
    for dev in dev_dict['devices']:
        if (args.verbose): print('create_device_cfg:', dev)
        # skip if network device other than tty does not mention local_enclve_name (needed for ilip)
        if ( (dev['comms'] != "ipc") and (dev['comms'] != "zmq") and (dev['comms'] != "tty") ):
            if (local_enclve_name not in dev.values()):
                if (args.verbose): print('skip this device in enclave', local_enclve_name)
                break
        d={}
        if 'enabled' in dev: d['enabled'] = dev['enabled']
        else:                d['enabled'] = 1
        d['id']         = 'xdd'+str(index)
        if 'path' in dev: d['path'] = dev['path']
        d['model']      = dev['model']
        d['comms']      = dev['comms']
        if ((d['comms'] == "udp") or (d['comms'] == "tcp")):
            add_inet(local_enclve_name, d, dev)
        elif (d['comms'] == "ilp"):
            add_ilp(local_enclve_name, d, dev)
        elif ( (d['comms'] == "ipc") or (d['comms'] == "zmq") ):
            add_ipc(local_enclve_name, d, dev, enc_info)
        elif (d['comms'] == "tty"):
            add_tty(local_enclve_name, d, dev)
        index += 1
        hal_config_dev_list.append(dict(d))
    if (args.verbose): print ('\nNET Device(s) list (', index, 'devices ) =', hal_config_dev_list)
    if (index < 2):
      print('\nEXITING: Could not find device info about enclave', local_enclve_name)
      sys.exit()
    return (hal_config_dev_list)

# Create HAL maps as a list of python dictionaries
def create_maps(local_enclve_name, halmaps, dev_list):
    xdc_dev_list = []
    for dev in dev_list:
        # TODO - select which HAL device. For now, assume there is just one of each is enabled.
        if (dev['enabled'] == 1):
            if ( (dev['comms'] == "ipc") or (dev['comms'] == "zmq") ): dev_app = dev['id']
            else:
              dev_net = dev['id']
              xdc_dev_list.append(dev['id'])
    if (args.verbose): print ('create maps for enclave', local_enclve_name, 'between', dev_app, 'and', dev_net)
    if (len(xdc_dev_list) != 1):
        print ('Exit: Must have exactly one xdc device enabled, not', len(xdc_dev_list), xdc_dev_list)
        sys.exit(1)
        
    hal_config_map_list = []
    for map in halmaps:
        d={}
#        print('map =', map)
        d['from_mux'] = map['mux']
        d['to_mux']   = map['mux']
        d['from_sec'] = map['sec']
        d['to_sec']   = map['sec']
        d['from_typ'] = map['typ']
        d['to_typ']   = map['typ']
        d['codec']    = 'NULL'
        
        if (local_enclve_name == map['from']):
            d['to_dev']   = dev_net
            d['from_dev'] = dev_app
        else:
            d['to_dev']   = dev_app
            d['from_dev'] = dev_net
#        print ('d =', d)
        hal_config_map_list.append(dict(d))
        
    if (args.verbose): print('MAPs =', hal_config_map_list)
    return(hal_config_map_list)

###############################################################
# Combine dev and map lists into dictionaries of tuples
#   libconfig lists, enclosed by () parentheses, map to Python
#     tuples and can contain arbitrary values
#   libconfig array, enclosed by [] brackets, map to Python
#     lists and must use scalars values of the same type
#     (int, float, bool, string)
###############################################################
def combine_lists_into_dict(dev_list, map_list):
    dev_dict  = {'devices': tuple(dev_list)}
#    if (args.verbose):  print(libconf.dumps(dev_dict))
    map_dict  = {'maps': tuple(map_list)}
#    if (args.verbose):  print(libconf.dumps(map_dict))
    map_dict.update(dev_dict)
    return(map_dict)

###############################################################
# Main program
###############################################################
if __name__=='__main__':
    args = get_args()
    map_dict = read_json(args.json_api_file)
    dev_dict = read_json(args.json_devices_file)
    op = args.output_file_prefix + '_'
    if args.output_dir: op = args.output_dir + '/' + op
    for enc_info in map_dict['enclaves']:
        e = enc_info['enclave']
        if (args.verbose):  print ('\nProcessing enclave', e)
        dev_list = create_device_cfg(dev_dict, enc_info, e)
        map_list = create_maps(e, enc_info['halmaps'], dev_list)
        cfg_dict = combine_lists_into_dict(dev_list, map_list)
        write_hal_config_file(op + e + '.cfg', cfg_dict)
