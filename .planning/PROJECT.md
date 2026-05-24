# Project: darktable Auto-Rating (AI Aesthetic Scoring)

## Overview
Add one-click AI-powered automatic star rating to darktable. The feature uses an aesthetic scoring model to evaluate image quality and automatically assigns 1-5 star ratings to selected images.

## Context
- **Base project**: darktable (RAW photo workflow application)
- **Branch**: dev
- **Language**: C (GTK3 GUI, ONNX Runtime AI backend)

## Key Constraints
1. **Minimal upstream conflicts**: New code in isolated files; minimal changes to existing files
2. **AI toggle**: Feature only available when "enable AI features" is ON in preferences
3. **Model-based**: Uses darktable's existing AI model registry (task = "rating")
4. **Commit style**: Minimize commit count, maximize files per commit, detailed descriptions

## Architecture

### Existing AI Infrastructure (leveraged)
- `src/common/ai_models.h/c` — Model registry, download, enable/disable
- `src/ai/backend.h` — ONNX Runtime inference API
- `src/gui/preferences_ai.c` — AI settings UI (no changes needed)
- `data/ai_models.json` — Model registry JSON

### New Components
- `src/common/ai/aesthetic_rating.h/c` — Aesthetic scoring inference module
- `src/libs/tools/auto_rating.c` — Toolbar button lib module

### Modified Files (minimal)
- `data/ai_models.json` — Add rating model entry
- `src/common/ai/CMakeLists.txt` — Add new source file
- `src/libs/tools/CMakeLists.txt` — Add new lib module

## UI Placement
Button placed in `DT_UI_CONTAINER_PANEL_CENTER_TOP_LEFT` toolbar, after the image count label. Uses star icon (`dtgtk_cairo_paint_star`). Tooltip: "auto-rate selected images with AI".

## Model Contract
- **Task**: `rating`
- **Input**: RGB image (uint8 HWC), resized to model's expected input size
- **Output** (configurable via model attribute):
  - `nima_output = false` (default): Single float in [0, 1] range
  - `nima_output = true`: 10-class probability distribution (scores 1–10)
- **Mapping**: weighted average -> normalize to [0,1] -> 1-5 stars (quintile)

## Model Requirements (external)
- **Primary**: NIMA (Neural Image Assessment) converted to ONNX
  - Outputs 10 probabilities p[1]..p[10] for AVA dataset scores
  - Weighted average = Σ(p[i] × i), then normalize to [0,1]
- **Fallback**: Any single-float aesthetic scorer with `nima_output = false`

Model packaging: `.dtmodel` zip containing `model.onnx` + `config.json` with attributes:
- `input_size` (int, default 224)
- `normalize` (bool, default true = ImageNet normalization)
- `nima_output` (bool, default false)

## Development Environment Setup (macOS)

> 本项目在移动硬盘上开发，需支持换机器快速重建环境。

### 1. 系统依赖（Homebrew）

系统级库通过 Homebrew 统一管理，无法放入项目目录：

```bash
brew bundle --file=Brewfile
```

Brewfile 中包括：GTK3、Glib、Exiv2、LensFun、LCMS2、OpenMP (libomp) 等。

### 2. 项目内依赖（自动下载）

运行项目根目录的 setup 脚本，自动完成以下步骤：

```bash
./setup-build.sh
```

脚本行为：
1. 执行 `brew bundle` 安装系统依赖
2. 下载 ONNX Runtime 到 `build/_deps/onnxruntime/`
3. 运行 cmake 配置（含 OpenMP、Pugixml bundled 等参数）

### 3. 手动 cmake 配置（如需自定义）

```bash
cd build
cmake .. \
  -DUSE_AI=ON \
  -DUSE_BUNDLED_PUGIXML=OFF \
  -DDONT_USE_INTERNAL_LUA=Off \
  -DUSE_HEIF=OFF \
  -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp -I$(brew --prefix libomp)/include" \
  -DOpenMP_C_LIB_NAMES="omp" \
  -DOpenMP_omp_LIBRARY="$(brew --prefix libomp)/lib/libomp.dylib" \
  -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp -I$(brew --prefix libomp)/include" \
  -DOpenMP_CXX_LIB_NAMES="omp"
```

### 4. 编译

```bash
cd build
make -j$(sysctl -n hw.ncpu)
```

### 换机器 workflow

```bash
git clone <repo> /path/to/darktable
cd /path/to/darktable
./setup-build.sh
cd build && make -j8
```
