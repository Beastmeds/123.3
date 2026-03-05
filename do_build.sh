#!/bin/bash
cd /workspaces/123.3
make clean >/tmp/build_clean.log 2>&1
make 2>&1 | tee /tmp/build_result.log
echo "=== BUILD EXIT CODE: $? ===" >> /tmp/build_result.log
cat /tmp/build_result.log
