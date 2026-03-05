#!/usr/bin/env python3
import subprocess
import os
import sys

os.chdir('/workspaces/123.3')

print("=== Cleaning build artifacts ===")
subprocess.run(['make', 'clean'], capture_output=True)

print("\n=== Starting build ===")
result = subprocess.run(['make', '-j1'], capture_output=True, text=True, timeout=120)

print("STDOUT:")
print(result.stdout[:5000])

if result.stderr:
    print("\nSTDERR:")
    print(result.stderr[:5000])

print("\nReturn code:", result.returncode)

if result.returncode == 0:
    print("\n✓ Build successful!")
    if os.path.exists('myos.img'):
        size = os.path.getsize('myos.img')
        print(f"✓ myos.img created ({size} bytes)")
else:
    print("\n✗ Build failed")
    
sys.exit(result.returncode)
