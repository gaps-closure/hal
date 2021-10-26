#! /usr/bin/env python3
import argparse
from dataclasses import dataclass
from shutil import copyfile, copytree
from pathlib import Path
import subprocess
import sys
import os
from typing import Dict, Type
import build

def install_hal_daemon(out: Path) -> None:
    path = Path('daemon')
    out_bin = out / 'bin' 
    out_bin.mkdir(parents=True, exist_ok=True)
    copyfile(path / 'hal', out_bin / 'hal')
    os.chmod(out_bin / 'hal', 0o755)

def install_python_package(out: Path) -> None:
    subprocess.run([sys.executable, '-m', 'pip', 'install', '.', '--upgrade', '--target', out / 'python'])   

def install_hal_includes(out: Path) -> None:
    out_include = out / 'include' 
    out_include.mkdir(parents=True, exist_ok=True)
    copyfile(Path('api') / 'xdcomms.h', out_include / 'xdcomms.h')
    copyfile(Path('log') / 'log.h', out_include / 'log.h')

def install_xdcomms_lib(out: Path) -> None:
    out_lib = out / 'lib' 
    out_lib.mkdir(parents=True, exist_ok=True)
    copyfile(Path('api') / 'libxdcomms.so', out_lib / 'libxdcomms.so')

def install_device_defs(out: Path) -> None:
    out_etc = out / 'etc' 
    copytree(Path('confgen') / 'device_defs', out_etc, dirs_exist_ok=True)

@dataclass
class Args:
    output: Path

def install(args: Type[Args]) -> Dict[str, str]:
    install_hal_daemon(args.output)
    install_hal_includes(args.output)
    install_xdcomms_lib(args.output)
    install_device_defs(args.output)
    install_python_package(args.output)
    return {
        "PATH": f"{args.output.resolve()}/bin:{args.output.resolve()}/python/bin",
        "PYTHONPATH": f"{args.output.resolve()}/python",
    }

def main() -> None: 
    parser = argparse.ArgumentParser('install.py') 
    parser.add_argument('--output', '-o', default=False, help="Output directory", type=Path, required=True)
    args = parser.parse_args(namespace=Args)
    args.output.mkdir(parents=True, exist_ok=True)
    build.build()
    install(args)
    
if __name__ == '__main__':
    main()