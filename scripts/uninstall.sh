#!/bin/bash
# X# (Xsharp) Uninstall Script

set -e

PREFIX="${PREFIX:-/usr/local}"

echo "Uninstalling X# from ${PREFIX}..."

sudo rm -f "${PREFIX}/bin/xsharp"
sudo rm -f "${PREFIX}/bin/xsharp-ide"
sudo rm -rf "${PREFIX}/lib/xsharp"
sudo rm -rf "${PREFIX}/include/xsharp"
sudo rm -rf "${PREFIX}/share/xsharp"

echo "X# has been uninstalled."
