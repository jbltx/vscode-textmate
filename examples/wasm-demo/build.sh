#!/bin/bash

# VSCode TextMate WASM Demo Build Script
# Builds the WASM module and sets up the demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WASM_BUILD_DIR="$PROJECT_ROOT/textmate-cpp/build-wasm-demo"
DEMO_DIST_DIR="$SCRIPT_DIR/dist"

echo "🔨 Building VSCode TextMate WASM Demo"
echo ""

# Check if WASM files already exist
if [ -f "$DEMO_DIST_DIR/textmate-standard.wasm" ] && [ -f "$DEMO_DIST_DIR/textmate-standard.js" ]; then
    echo "✓ WASM files already exist in $DEMO_DIST_DIR"
    echo ""
    echo "Starting web server..."
    echo "📺 Open http://localhost:8000 in your browser"
    echo ""
    cd "$SCRIPT_DIR"
    python3 -m http.server 8000
    exit 0
fi

echo "Step 1: Checking Emscripten..."
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found"
    echo "Activate with: source ~/dev/emsdk/emsdk_env.sh"
    exit 1
fi

EMSCRIPTEN_VERSION=$(emcc --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
echo "✓ Emscripten $EMSCRIPTEN_VERSION detected"
echo ""

echo "Step 2: Building WASM module..."
cd "$PROJECT_ROOT/textmate-cpp"

rm -rf "$WASM_BUILD_DIR"
mkdir -p "$WASM_BUILD_DIR"
cd "$WASM_BUILD_DIR"

echo "  Configuring CMake..."
source ~/dev/emsdk/emsdk_env.sh 2>/dev/null || true

emcmake cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_WASM_BUILD=ON \
    -DWASM_VARIANT=standard \
    .. > /dev/null 2>&1

echo "  Building..."
cmake --build . -- -j$(nproc) > /dev/null 2>&1

if [ ! -f "textmate-standard.wasm" ] || [ ! -f "textmate-standard.js" ]; then
    echo "❌ WASM build failed"
    exit 1
fi

echo "✓ WASM module built successfully"
echo ""

echo "Step 3: Setting up demo..."
mkdir -p "$DEMO_DIST_DIR"
cp textmate-standard.{js,wasm} "$DEMO_DIST_DIR/"

# Get file sizes
JS_SIZE=$(stat -f%z "$DEMO_DIST_DIR/textmate-standard.js" 2>/dev/null || stat -c%s "$DEMO_DIST_DIR/textmate-standard.js")
WASM_SIZE=$(stat -f%z "$DEMO_DIST_DIR/textmate-standard.wasm" 2>/dev/null || stat -c%s "$DEMO_DIST_DIR/textmate-standard.wasm")

JS_SIZE_KB=$((JS_SIZE / 1024))
WASM_SIZE_KB=$((WASM_SIZE / 1024))

echo "✓ Demo set up successfully"
echo ""
echo "📦 Generated files:"
echo "  - textmate-standard.js   ($JS_SIZE_KB KB)"
echo "  - textmate-standard.wasm ($WASM_SIZE_KB KB)"
echo ""

echo "✅ Build complete!"
echo ""
echo "🚀 Starting web server..."
echo "📺 Open http://localhost:8000 in your browser"
echo ""

cd "$SCRIPT_DIR"
python3 -m http.server 8000
