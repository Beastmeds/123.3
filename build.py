#!/usr/bin/env python3
import subprocess
import sys
import os

os.chdir('/workspaces/123.3')
result = subprocess.run(['make', '-j4'], capture_output=True, text=True, timeout=60)
print(result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)
sys.exit(result.returncode)
