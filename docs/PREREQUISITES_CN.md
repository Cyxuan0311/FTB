# 外部依赖

FTB 使用外部 CLI 工具来提供增强的预览功能。所有工具都是**可选的**——如果未安装某个工具，对应的预览类型会优雅地降级或跳过。

## 预览工具

| 工具 | 预览类型 | 安装方式 |
|------|---------|---------|
| [timg](https://github.com/hzeller/timg) | 媒体预览（GIF、WebP、视频） | `apt install timg` |
| [ffmpeg](https://ffmpeg.org) | 视频首帧提取（与 timg 配合使用） | `apt install ffmpeg` |
| [glow](https://github.com/charmbracelet/glow) | Markdown 预览 / DOCX 渲染 | `apt install glow` |
| [xleak](https://github.com/anomalyco/xleak) | 电子表格预览（xlsx、xls、ods） | `cargo install xleak` |
| [eyeD3](https://eyed3.readthedocs.io) | 音频元数据显示 | `apt install eyed3` |
| [hygg](https://github.com/anomalyco/hygg) | PDF 文本提取 | `cargo install hygg` |
| [pandoc](https://pandoc.org) | DOC/DOCX 转 Markdown | `apt install pandoc` |
| [catdoc](http://www.wagner.pp.ru/~vitus/software/catdoc/) | 旧版 `.doc` 文本提取 | `apt install catdoc` |
| [xxd](https://linux.die.net/man/1/xxd) | 十六进制转储（vim-common 的一部分） | （通常已预装） |

## 归档列表

FTB 按层级降级顺序使用以下工具列出归档内容：

| 工具 | 归档格式 | 安装方式 |
|------|---------|---------|
| [unzip](https://infozip.sourceforge.net) | `.zip` | `apt install unzip` |
| [tar](https://www.gnu.org/software/tar/) | `.tar`、`.tgz`、`.tbz2`、`.txz`、`.tzst` | （通常已预装） |
| [7z](https://www.7-zip.org) | `.7z`、`.rar`、`.cab` | `apt install p7zip-full` |
| [isoinfo](https://www.gnu.org/software/xorriso/) | `.iso` | `apt install genisoimage` |

## 其他工具

| 工具 | 功能 | 安装方式 |
|------|---------|---------|
| [qjs (QuickJS)](https://bellard.org/quickjs/) | 插件系统 JavaScript 运行时 | `apt install quickjs` 或从源码编译 |
| [fdfind](https://github.com/sharkdp/fd) / `fd` | FuzzyFinder 中更快的文件搜索 | `apt install fd-find` |

## 快速安装（Ubuntu/Debian）

```bash
# 预览工具
sudo apt-get install -y timg ffmpeg glow eyed3 pandoc catdoc xxd unzip p7zip-full genisoimage

# 其他
sudo apt-get install -y quickjs fd-find
```

对于 `xleak` 和 `hygg`（Rust 工具）：
```bash
cargo install xleak hygg
```
