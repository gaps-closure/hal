#! /usr/bin/env python3
import argparse
from dataclasses import dataclass
from shutil import copyfile
from pathlib import Path
import subprocess
import sys
from typing import Dict, Type
import build

def install_hal_daemon(out: Path) -> None:
    path = Path('daemon')
    out_bin = out / 'bin' 
    out_bin.mkdir(parents=True, exist_ok=True)
    copyfile(path / 'hal', out_bin / 'hal')

def install_python_package(out: Path) -> None:
    subprocess.run([sys.executable, '-m', 'pip', 'install', '.', '--upgrade', '--target', out])   

def make_env(out: Path) -> None:
    with open(out / 'closureenv', 'a') as env_f: 
        env_f.write("\n".join([
            f'export PYTHONPATH={out.resolve()}:$PYTHONPATH',
            f'export PATH={out.resolve()}/bin:$PATH'
        ]))

@dataclass
class Args:
    output: Path

def install(args: Type[Args]) -> Dict[str, str]:
    install_hal_daemon(args.output)
    install_python_package(args.output)
    return {
        "PATH": f"{args.output.resolve()}/bin",
        "PYTHONPATH": f"{args.output.resolve()}/bin",
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