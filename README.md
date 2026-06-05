# darktable + AI Auto-Rating

基于 [darktable](https://www.darktable.org/) 开源摄影工作流软件的自定义分支，加入 AI 一键评分功能。

## 项目简介

本项目在 darktable 的基础上集成了 AI 美学评分系统，利用 ONNX Runtime 运行美学评分模型，对选中的照片自动分配星级评分（Reject / 0-5 星）。评分基于可配置的阈值区间，用户可在偏好设置中精细调整各星级对应的评分阈值。

### 核心功能

- **一键评分**：在 lighttable 界面选中照片后，点击评分按钮或使用快捷键，自动为所有选中照片分配星级
- **美学评分模型**：支持 NIMA 及通用单输出评分模型，通过 ONNX Runtime 进行推理
- **可配置阈值**：在偏好设置 → AI 中，可为每个星级设置评分阈值，启用/禁用特定星级，并支持滚轮微调与直接数值输入
- **阈值排序保护**：自动维护相邻启用阈值之间的最小间隔，防止评分区间重叠
- **模型管理**：支持从远程下载、从本地 `.dtmodel` 文件导入模型，以及模型详情查看

## 构建

本项目沿用 darktable 的构建系统，需额外启用 AI 功能：

```bash
git clone --recurse-submodules <repo-url>
cd darktable
git submodule update --init
```

### macOS (Homebrew)

```bash
# 安装依赖
./tools/setup-build.sh

# 构建（含 AI 功能）
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_AI=ON ..
cmake --build . --target darktable
```

### Linux

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_AI=ON -DCMAKE_INSTALL_PREFIX=/opt/darktable ..
cmake --build .
sudo cmake --install .
```

详细的构建选项和依赖说明请参考上游 darktable 文档。

## 运行

### macOS 开发版

```bash
./tools/launch.sh
```

脚本会自动设置 `GSETTINGS_SCHEMA_DIR` 等环境变量并启动构建产物。

**注意**：启动开发版前请先关闭系统安装的 darktable，避免资源冲突。

### 安装后运行

```bash
/opt/darktable/bin/darktable
```

## 使用方法

1. 打开 **偏好设置 → AI**，启用 AI 功能
2. 在 **Models** 列表中下载或导入评分模型，勾选启用
3. 选中一个 rating 类型的模型后，下方出现 **Rating Thresholds** 面板
4. 根据需要调整各星级的阈值区间（滚轮微调，或直接在右侧输入框键入数值）
5. 回到 lighttable，选中照片，点击评分按钮即可自动评分

## AI 推理加速

CPU 推理开箱即用。GPU 加速需要额外安装 GPU 版 ONNX Runtime：

| 平台 | 加速方式 | 说明 |
|------|---------|------|
| macOS (Apple Silicon) | CoreML | 内置，自动使用 Neural Engine |
| Windows | DirectML | 内置，支持 DirectX 12 GPU |
| Linux / Windows (NVIDIA) | CUDA | 需安装 CUDA + cuDNN |
| Linux (AMD) | ROCm | 需安装 ROCm + MIGraphX |

在偏好设置 → AI 中点击 **detect** 可自动检测系统已安装的 ONNX Runtime 库。

## 技术架构

```
src/
├── common/ai/
│   ├── aesthetic_rating.c/h    # 美学评分推理引擎
│   ├── restore.c/h             # AI 图像修复
│   ├── segmentation.c/h        # AI 图像分割
│   └── ...
├── gui/
│   └── preferences_ai.c        # AI 偏好设置面板（含阈值配置）
├── libs/tools/
│   └── auto_rating.c           # lighttable 评分按钮模块
└── ...
```

## 与上游 darktable 的关系

本项目 fork 自 darktable 上游仓库，保持了与上游的兼容性。darktable 原有的所有功能（RAW 开发、图片管理、编辑等）完整保留。

上游 darktable 信息：
- 官网：https://www.darktable.org/
- 源码：https://github.com/darktable-org/darktable
- 文档：https://github.com/darktable-org/dtdocs
- 许可证：GPL-3.0-or-later

## 许可证

本项目遵循 darktable 的原始许可证：**GPL-3.0-or-later**。
