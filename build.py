#! /usr/bin/env python3
import argparse
import subprocess
from dataclasses import dataclass
from pathlib import Path

@dataclass
class Args:
    clean: bool

def build() -> None:
    subprocess.check_call(['make', '-j', '8'])    

def clean() -> None:
    subprocess.check_call(['make', 'clean'])    

def main() -> None: 
    parser = argparse.ArgumentParser('build.py') 
    parser.add_argument('--clean', '-c', action='store_true', default=False)
    args = parser.parse_args(namespace=Args)
    if args.clean:
        clean()
    else:
        build()

if __name__ == '__main__':
    main()