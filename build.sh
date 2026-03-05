#!/bin/bash
cd /workspaces/123.3
make clean 2>&1 > /tmp/build.log
make 2>&1 >> /tmp/build.log
echo "Build complete. Return code: $?" >> /tmp/build.log
cat /tmp/build.log
