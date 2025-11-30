#!/bin/bash
# Script to build Debian package for Slippage

set -e

echo "Building Slippage Debian package..."

# Check for required tools
if ! command -v dpkg-buildpackage &> /dev/null; then
    echo "Error: dpkg-buildpackage not found. Install with:"
    echo "  sudo apt-get install dpkg-dev"
    exit 1
fi

if ! command -v debuild &> /dev/null; then
    echo "Warning: debuild not found. Install devscripts for signing:"
    echo "  sudo apt-get install devscripts"
    echo "Building unsigned package..."
    dpkg-buildpackage -us -uc -b
else
    # Build unsigned package (remove -us -uc to sign)
    dpkg-buildpackage -us -uc -b
fi

echo ""
echo "Build complete! Package created in parent directory:"
ls -lh ../slippage_*.deb

echo ""
echo "To install:"
echo "  sudo dpkg -i ../slippage_*.deb"
