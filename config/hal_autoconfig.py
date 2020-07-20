#!/usr/bin/env python3

# Create HAL configuration file
#    July 17, 2020
#
# Usage Examples:
#  python3 hal_autoconfig
#  python3 hal_autoconfig.py -d devices_6modemo_bw.json -x xdconf_6modemo_bw.json

import json
import argparse
import libconf
import sys
        
def get_args():
    parser = argparse.ArgumentParser(description='Create HAL configuration file')
    parser.add_argument('-d', '--json_devices_file',  help='Input JSON file name of HAL device conig', type=str, default='devices_example1.json')
    parser.add_argument('-o', '--output_file_prefix', help='Output HAL configuration file name prefix', type=str, default='hal')
    parser.add_argument('-v', '--verbose', help="run in verbose mode", action='store_true', default=False)
    parser.add_argument('-x', '--json_api_file',      help='Input JSON file name of API config and HAL tag-maps', type=str, default='xdconf.ini.json')
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
        print ('Writing HAL libconf configuration file to' , file_name)
        libconf.dump(dict, f)

###############################################################
# Put Config File device and map information into a dictionary
###############################################################
# Create HAL API device config(s) as a list of python dictionaries
def create_device_cfg_ap(index, dev_dict, inuri, outuri):
    hal_config_dev_list = []
    for dev in dev_dict['app']:
        d={}
        d['enabled']    = dev['enabled']
        d['id']         = 'xdd'+str(index)
        d['path']       = dev['path']
        d['model']      = dev['model']
        d['comms']      = dev['comms']
        d['mode_in']    = dev['mode_in']
        d['mode_out']   = dev['mode_out']
        d['addr_in']    = outuri;
        d['addr_out']   = inuri;
#        print ('d =', dev, '\n', libconf.dumps(d))
        index += 1
        hal_config_dev_list.append(dict(d))
    if (args.verbose): print ('\nAPP Device(s) list =', hal_config_dev_list)
#    print (libconf.dumps(list[0]))
    return (hal_config_dev_list, index)

# Create HAL Network device config(s) as a list of python dictionaries
def create_device_cfg_bw(index, dev_dict, local_enclve_name):
    hal_config_dev_list = []
    for dev in dev_dict['network']:
        d={}
        d['enabled']    = dev['enabled']
        d['id']         = 'xdd'+str(index)
        d['path']       = dev['path']
        d['model']      = dev['model']
        d['comms']      = dev['comms']
        if (local_enclve_name == dev['enclave_name_1']):
            d['addr_in']    = dev['listen_addr_1']
            d['port_in']    = dev['listen_port_1']
            if 'connect_addr_1' in dev:
                print ("found1")
                d['addr_out']   = dev['connect_addr_1']
                d['port_out']   = dev['connect_port_1']
            else:
                d['addr_out']   = dev['listen_addr_2']
                d['port_out']   = dev['listen_port_2']
        else:
            d['addr_in']    = dev['listen_addr_2']
            d['port_in']    = dev['listen_port_2']
            if 'connect_addr_1' in dev:
                print ("found2")
                d['addr_out']   = dev['connect_addr_2']
                d['port_out']   = dev['connect_port_2']
            else:
                d['addr_out']   = dev['listen_addr_1']
                d['port_out']   = dev['listen_port_1']
        index += 1
        hal_config_dev_list.append(dict(d))
    if (args.verbose): print ('\nNET Device(s) list =', hal_config_dev_list)
    return (hal_config_dev_list, index)

def create_devices(enc_info, dev_dict):
    if (args.verbose): print  ('\nINPUT DEVICE DICTIONARY =', dev_dict)
    dev_tuple = ()
    index=0
    dev_list_apps, index = create_device_cfg_ap(index, dev_dict, enc_info['inuri'], enc_info['outuri'])
    dev_list_nets, index = create_device_cfg_bw(index, dev_dict, enc_info['enclave'])
    return(dev_list_apps, dev_list_nets)

def create_maps(local_enclve_name, halmaps, dev_list_apps, dev_list_nets):
    print ('create maps for enclave', enc_info['enclave'])
    
    # TODO - select which HAL device. For now, assume there is just one of each.
    dev_app = dev_list_apps[0]['id']
    dev_net = dev_list_nets[0]['id']

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
    return(hal_config_map_list)
        

def combine_lists_into_dict(dev_list_apps, dev_list_nets, map_list):
    # libconfig Lists, enclosed by () parenthesis, map to Python tuples and can contain arbitrary values
    # libconfig array, enclosed by [] brackets, map to Python lists and must use scalars values
    #   (int, float, bool, string) of the same type.
    dev_tuple = tuple(dev_list_apps + dev_list_nets)
    dev_dict  = {'devices': dev_tuple}
    if (args.verbose):  print(libconf.dumps(dev_dict))
        
    map_tuple = tuple(map_list)
    map_dict  = {'maps': map_tuple}
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
        print ('\nProcessing enclave', e)
        dev_list_apps, dev_list_nets = create_devices(enc_info, dev_dict)
        map_list = create_maps(e, enc_info['halmaps'], dev_list_apps, dev_list_nets)
        libconf_dict = combine_lists_into_dict(dev_list_apps, dev_list_nets, map_list)
        write_hal_config_file(args.output_file_prefix + '_' + e + '.cfg', libconf_dict)
