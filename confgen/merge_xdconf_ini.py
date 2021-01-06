#!/usr/bin/python3
import json
from   argparse      import ArgumentParser

def get_args():
  p = ArgumentParser(description='Merge CLOSURE xdconf.ini files')
  p.add_argument('-f', '--files', required=True, type=str, help='Input files')
  p.add_argument('-o', '--outfile', required=False, type=str, default='xdconf.ini', help='Output file')
  return p.parse_args()

def main():
  args   = get_args()
  print('Options selected:')
  for x in vars(args).items(): print('  %s: %s' % x)

  files=args.files.split(' ')
  if len(files) < 1:
    print('Require at least one file to merge')
    return

  data = {'enclaves': []}
  for f in files:
    with open(f,'r') as inf:
      cur = json.load(inf)
    enc = cur['enclaves']
    for e in enc:
      # find matching enclave e1 in data['enclaves']
      found = False;
      for e1 in data['enclaves']:
        if e['enclave'] == e1['enclave']:
          found = True;
          break;
      # if e not in data['enclaves'], simply add enclave to data['enclaves']
      if not found:
        data['enclaves'].append(e)
      else:
        if e['inuri'] != e1['inuri'] or e['outuri'] != e1['outuri']:
          print('URI do not match, merge not possible')
          exit
        # XXX: need to check for duplicates
        print("Warning: Not checking for duplicate halmaps")
        e1['halmaps'].extend(e['halmaps'])

  with open(args.outfile, 'w') as outf:
    json.dump(data,outf,indent=2)
  
if __name__ == '__main__':
  main()
