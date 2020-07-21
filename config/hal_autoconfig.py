#!/usr/bin/env python3

# Create HAL configuration file
#    July 22, 2020
#
# Usage Examples:
#  1) eri demo running in emulator (start.sh example1) with BE devices (default inputs)
#       python3 hal_autoconfig.py
#  2) eri demo (using default xdconf: xdconf_eri.json) with BW devices
#       python3 hal_autoconfig -d devices_eri_bw.json
#  3) 6month demo running both enclave on jaga (with sudo bash test/6MoDemo_BW.net.sh)
#       python3 hal_autoconfig.py -x xdconf_6modemo.json -d devices_6modemo_bw.json
#  4) 6month demo running both enclave on jaga (with ilip root, read and write devices)
#       python3 hal_autoconfig.py -x xdconf_6modemo.json -d devices_6modemo_be.json

import json
import argparse
import libconf
import sys
        
def get_args():
    parser = argparse.ArgumentParser(description='Create HAL configuration file')
    parser.add_argument('-d', '--json_devices_file',  help='Input JSON file name of HAL device conig', type=str, default='devices_eri_be.json')
    parser.add_argument('-o', '--output_file_prefix', help='Output HAL configuration file name prefix', type=str, default='hal')
    parser.add_argument('-v', '--verbose', help="run in verbose mode", action='store_true', default=False)
    parser.add_argument('-x', '--json_api_file',      help='Input JSON file name of HAL API and tag-maps', type=str, default='xdconf_eri.json')
    return parser.parse_args()

# Read JSON file into a python dictionary (data)
def read_json(file_name):
    try:
        with open(file_name, 'r') as json_file:
            data = json.load(json_file)
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
# Add ipc speciffic infofrmation to mutable dictionaty (d)
def add_ipc(local_enclve_name, d, dev, enc_info):
    d['mode_in']    = dev['mode_in']
    d['mode_out']   = dev['mode_out']
    d['addr_in']    = enc_info['outuri']
    d['addr_out']   = enc_info['inuri']
    
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
    print('Add serial device info to mutable dictionaty in', local_enclve_name)
    if (local_enclve_name == dev['enclave_name_1']):
        d['path_r']    = dev['path_r_1']
        d['path_w']    = dev['path_w_1']
        d['from_mux']  = dev['from_mux_1']
        if (dev['init_at_1'] > 0):
            d['init_enable']  = 1
        else:
            d['init_enable']  = 0
    print('TODO: special case for using root device to initialize read/write devices')
    print ('d =', d)
    sys.exit()
            
# Create HAL device configurations as a list of python dictionaries
def create_device_cfg(dev_dict, enc_info, local_enclve_name):
    index=0
    hal_config_dev_list = []
    for dev in dev_dict['devices']:
        d={}
        if 'enabled' in dev: d['enabled'] = dev['enabled']
        else:                d['enabled'] = 1
        d['id']         = 'xdd'+str(index)
        d['path']       = dev['path']
        d['model']      = dev['model']
        d['comms']      = dev['comms']
        if ((d['comms'] == "udp") or (d['model'] == "tcp")):
            add_inet(local_enclve_name, d, dev)
        elif (d['model'] == "ilp"):
            add_ilp(local_enclve_name, d, dev)
        elif (d['comms'] == "ipc"):
            add_ipc(local_enclve_name, d, dev, enc_info)
        index += 1
        hal_config_dev_list.append(dict(d))
    if (args.verbose): print ('\nNET Device(s) list =', hal_config_dev_list)
    return (hal_config_dev_list)

# Create HAL maps as a list of python dictionaries
def create_maps(local_enclve_name, halmaps, dev_list):
    for dev in dev_list:
        # TODO - select which HAL device. For now, assume there is just one of each.
        if (dev['comms'] == "ipc"): dev_app = dev['id']
        else:                       dev_net = dev['id']
    if (args.verbose): print ('create maps for enclave', local_enclve_name, 'between', dev_app, 'and', dev_net)

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
# Combine dev and map lists into dictionariesof off tuples
#   libconfig lists, enclosed by () parentheses, map to Python
#     tuples and can contain arbitrary values
#   libconfig array, enclosed by [] brackets, map to Python
#     lists and must use scalars values of the same type
#     (int, float, bool, string)
###############################################################
def combine_lists_into_dict(dev_list, map_list):
    dev_dict  = {'devices': tuple(dev_list)}
    if (args.verbose):  print(libconf.dumps(dev_dict))
    map_dict  = {'maps': tuple(map_list)}
    if (args.verbose):  print(libconf.dumps(map_dict))
    map_dict.update(dev_dict)
    return(map_dict)

###############################################################
# Main program
###############################################################
if __name__=='__main__':
    args = get_args()
    map_dict = read_json(args.json_api_file)
    dev_dict = read_json(args.json_devices_file)
    for enc_info in map_dict['enclaves']:
        e = enc_info['enclave']
        if (args.verbose):  print ('\nProcessing enclave', e)
        dev_list = create_device_cfg(dev_dict, enc_info, e)
        map_list = create_maps(e, enc_info['halmaps'], dev_list)
        cfg_dict = combine_lists_into_dict(dev_list, map_list)
        write_hal_config_file(args.output_file_prefix + '_' + e + '.cfg', cfg_dict)
