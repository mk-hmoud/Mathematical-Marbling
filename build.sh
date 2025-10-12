#!/bin/bash

set -e

NPROC=$(nproc)

if [ "$1" == "clean" ]; then
    echo "==> Cleaning the 'utils' library..."
    if [ -f "utils/Makefile" ]; then
        (cd utils && make clean)
    fi

    echo "==> Cleaning the 'application' executable..."
    if [ -f "application/Makefile" ]; then
        (cd application && make clean)
    fi

    echo "==> Cleaning the 'rendering' library..."
    if [ -f "rendering/Makefile" ]; then
        (cd rendering && make clean)
    fi

    echo "==> Cleaning the 'marbling' library..."
    if [ -f "marbling/Makefile" ]; then
        (cd marbling && make clean)
    fi

    rm -rf lib bin
    echo "==> Cleanup complete."
    exit 0
fi

# Build the Utils library
echo "================================================="
echo "==> Building 'utils' static library (libutils.so)"
echo "================================================="
(cd utils && make -j"$NPROC")

# Build the Marbling library
echo "================================================="
echo "==> Building 'marbling' library (libmarbling.so)"
echo "================================================="
(cd marbling && make -j"$NPROC")

# Build the Rendering library
echo "================================================="
echo "==> Building 'rendering' library (librenderer.so)"
echo "================================================="
(cd rendering && make -j"$NPROC")

# Build the Application executable
echo "================================================="
echo "==> Building 'application' executable (marbling_app)"
echo "================================================="
(cd application && make -j"$NPROC")

echo ""
echo "================================================="
echo "==> Build successful!"
echo "==> Run the application with: ./application/bin/marbling_app"
echo "================================================="