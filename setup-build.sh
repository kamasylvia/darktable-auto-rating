#!/bin/bash
# Setup build environment for darktable on macOS
# Dependencies: cmake, Homebrew

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DEPS_DIR="${BUILD_DIR}/_deps"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== darktable build setup ===${NC}"

# Check Homebrew
if ! command -v brew &> /dev/null; then
    echo -e "${RED}Error: Homebrew is required. Install from https://brew.sh${NC}"
    exit 1
fi

# Install dependencies from Brewfile
echo -e "${YELLOW}Installing dependencies via Homebrew...${NC}"
brew bundle --file="${SCRIPT_DIR}/Brewfile"

# --- Generate built-in AI models (debug builds) ---
# Find a Python with TensorFlow support (prefer python3.11)
PYTHON_BIN=""
for py in python3.11 python3.10 python3.9 python3; do
    if command -v "$py" &> /dev/null && $py -c "import tensorflow" 2>/dev/null; then
        PYTHON_BIN="$(command -v "$py")"
        break
    fi
done

if [ -z "$PYTHON_BIN" ]; then
    echo -e "${YELLOW}No Python with TensorFlow found. Installing python@3.11 and dependencies...${NC}"
    brew install python@3.11
    PYTHON_BIN="$(brew --prefix python@3.11)/bin/python3.11"
    $PYTHON_BIN -m pip install --quiet --break-system-packages tensorflow tf2onnx 2>/dev/null || \
        $PYTHON_BIN -m pip install --quiet tensorflow tf2onnx
fi

MODELS_DIR="${SCRIPT_DIR}/data/models"
mkdir -p "$MODELS_DIR"

if [ ! -d "${MODELS_DIR}/rating-aesthetic-v1" ]; then
    echo -e "${YELLOW}Generating built-in NIMA aesthetic rating model (Inception ResNet v2)...${NC}"

    # Download Inception ResNet v2 weights from titu1994's release
    IR_WEIGHTS="${DEPS_DIR}/inception_resnet_weights.h5"
    if [ ! -f "$IR_WEIGHTS" ]; then
        echo -e "${YELLOW}Downloading Inception ResNet v2 weights (~208MB)...${NC}"
        # Primary: GitHub release
        curl -L -o "$IR_WEIGHTS" "https://github.com/titu1994/neural-image-assessment/releases/download/v0.5/inception_resnet_weights.h5" 2>/dev/null || true
        # Fallback: try gitcode mirror for China
        if [ ! -f "$IR_WEIGHTS" ] || [ ! -s "$IR_WEIGHTS" ]; then
            echo -e "${YELLOW}GitHub download failed, trying mirror...${NC}"
            curl -L -o "$IR_WEIGHTS" "https://gitcode.com/mirrors/titu1994/neural-image-assessment/releases/download/v0.5/inception_resnet_weights.h5" 2>/dev/null || true
        fi
        if [ ! -f "$IR_WEIGHTS" ] || [ ! -s "$IR_WEIGHTS" ]; then
            echo -e "${RED}Error: Failed to download Inception ResNet v2 weights${NC}"
            echo -e "${RED}Please manually download from: https://github.com/titu1994/neural-image-assessment/releases/tag/v0.5${NC}"
            exit 1
        fi
    fi

    # Generate model
    $PYTHON_BIN "${SCRIPT_DIR}/tools/generate_nima_model.py" \
        --backbone inception_resnet \
        --weights "$IR_WEIGHTS" \
        --output-dir "$MODELS_DIR" \
        --model-id rating-aesthetic-v1

    if [ -d "${MODELS_DIR}/rating-aesthetic-v1" ]; then
        echo -e "${GREEN}Built-in model generated: ${MODELS_DIR}/rating-aesthetic-v1${NC}"
    else
        echo -e "${YELLOW}Warning: Failed to generate built-in model${NC}"
    fi
else
    echo -e "${GREEN}Built-in model already exists${NC}"
fi

# Setup libomp paths for OpenMP
LIBOMP_PREFIX="$(brew --prefix libomp)"

# Download ONNX Runtime if not present
ONNX_VERSION="1.24.4"
ONNX_ARCHIVE="${DEPS_DIR}/onnxruntime-osx-arm64-${ONNX_VERSION}.tgz"
ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-osx-arm64-${ONNX_VERSION}.tgz"

mkdir -p "${DEPS_DIR}"

if [ ! -d "${DEPS_DIR}/onnxruntime/include" ]; then
    if [ ! -f "${ONNX_ARCHIVE}" ]; then
        echo -e "${YELLOW}Downloading ONNX Runtime ${ONNX_VERSION}...${NC}"
        curl -L -o "${ONNX_ARCHIVE}" "${ONNX_URL}"
    fi
    echo -e "${YELLOW}Extracting ONNX Runtime...${NC}"
    rm -rf "${DEPS_DIR}/onnxruntime"
    mkdir -p "${DEPS_DIR}/onnxruntime"
    tar -xzf "${ONNX_ARCHIVE}" -C "${DEPS_DIR}/onnxruntime" --strip-components=1
    echo -e "${GREEN}ONNX Runtime extracted to ${DEPS_DIR}/onnxruntime/${NC}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with cmake
echo -e "${YELLOW}Configuring with cmake...${NC}"
cmake "${SCRIPT_DIR}" \
    -DUSE_AI=ON \
    -DUSE_BUNDLED_PUGIXML=OFF \
    -DDONT_USE_INTERNAL_LUA=Off \
    -DUSE_HEIF=OFF \
    -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include" \
    -DOpenMP_C_LIB_NAMES="omp" \
    -DOpenMP_omp_LIBRARY="${LIBOMP_PREFIX}/lib/libomp.dylib" \
    -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include" \
    -DOpenMP_CXX_LIB_NAMES="omp" \
    "$@"

echo -e "${GREEN}=== Setup complete ===${NC}"
echo "Build directory: ${BUILD_DIR}"
echo "To build: cd ${BUILD_DIR} && make -j\$(sysctl -n hw.ncpu)"
