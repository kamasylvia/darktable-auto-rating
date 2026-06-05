# darktable AI Auto-Rating (Custom Fork)

## ⚠️ 区分两个 darktable

| | 系统已安装 | 本项目开发版 |
|---|---|---|
| **路径** | `/Applications/darktable.app/Contents/MacOS/darktable` | `/Volumes/UNITEK/Documents/Development/darktable/` |
| **来源** | Homebrew Cask (上游官方) | 本仓库，基于上游 darktable 加入了 AI 自动评分功能 |
| **启动方式** | `open -a darktable` | 编译后从 `build/bin/darktable` 启动 |
| **版本** | 5.2.1 (稳定版) | 开发分支，含自定义修改 |

**启动开发版本前，必须先关闭系统安装的 darktable**，避免端口/资源冲突。

## 环境变量（macOS）

开发版依赖 Homebrew 安装的 GTK/Glib，启动前必须设置：

```bash
export GSETTINGS_SCHEMA_DIR=/opt/homebrew/share/glib-2.0/schemas
```

缺少此变量会导致点击 settings 时崩溃：`GLib-GIO-ERROR: No GSettings schemas are installed on the system`。

## 启动开发版

```bash
./tools/launch.sh
```

`tools/launch.sh` 会自动设置所需的环境变量并启动开发版二进制文件。

## 项目简介

基于 darktable 开源照片工作流软件，加入 AI 自动星级评分功能：
- 使用 ONNX Runtime 运行美学评分模型
- 一键为选中照片自动分配 1-5 星
- 支持 NIMA 模型及通用单输出评分模型

## 提交准则

- **最小化提交数量**：每次提交应包含尽可能完整的逻辑单元，避免碎片化提交
- **最大化每个提交的文件数**：相关联的修改应放在同一个提交中
- **详细描述**：提交信息必须包含充分的上下文和说明，不能是单行的简单描述
