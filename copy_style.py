#!/usr/bin/env python3
import shutil
import sys
from pathlib import Path

src = Path(sys.argv[1])  
dst = Path(sys.argv[2]) 

if not src.exists():
    raise SystemExit(f"Erro: pasta {src} não existe.")

if dst.exists():
    shutil.rmtree(dst)

shutil.copytree(src, dst)
print(f"Copiado {src} → {dst}")

