# Docker 支持

FTB 提供了 Docker 支持，可以在隔离环境中运行，无需在宿主机上安装依赖。Dockerfile 采用多阶段构建，最终镜像体积小巧（约 33 MB 内容大小）。

## 前提条件

- [Docker](https://docs.docker.com/engine/install/)（24+）
- [Docker Compose](https://docs.docker.com/compose/install/)（v2+，Docker Desktop 已内置）

## 快速开始

```bash
# 使用 Docker Compose 构建并运行
docker compose build
docker compose run ftb /usr/local/bin/FTB /mnt/host
```

## 构建

### 直连构建（需能访问 GitHub）

```bash
docker compose build
```

### 使用镜像构建（国内推荐）

```bash
docker build \
  --build-arg GIT_MIRROR=https://gh-proxy.com/https://github.com \
  -t ftb .
```

`GIT_MIRROR` 参数用于指定 FTXUI 源码的 Git 镜像地址，默认值为 `https://github.com`。

## 运行

### 使用 Docker Compose

```bash
# 进入 bash shell
docker compose run ftb bash

# 直接启动 FTB
docker compose run ftb /usr/local/bin/FTB /mnt/host

# 在当前目录启动 FTB
docker compose run ftb /usr/local/bin/FTB $(pwd)
```

### 使用 Docker

```bash
docker run -it --rm \
  -v ~/.ftb:/root/.ftb \
  -v ~/:/mnt/host \
  ftb

# 自定义目录
docker run -it --rm \
  -v ~/.ftb:/root/.ftb \
  -v $(pwd):/mnt/host \
  ftb \
  /usr/local/bin/FTB /mnt/host
```

## 卷挂载

| 宿主机路径 | 容器内路径 | 说明 |
|-----------|-----------|------|
| `~/.ftb` | `/root/.ftb` | 持久化 FTB 配置文件 |
| `~/` | `/mnt/host` | 在容器内访问宿主机文件 |

## 构建参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `GIT_MIRROR` | `https://github.com` | 克隆 FTXUI 源码的 Git 镜像地址 |

## Dockerfile 结构

构建分为三个阶段：

1. **ftxui-builder** — 从源码编译 FTXUI v6.0.0
2. **builder** — 编译 FTB 并链接已构建的 FTXUI
3. **runtime** — 最小运行镜像，仅含 FTB 二进制和运行时依赖（tini）

## 提示

- 使用 `init: true`（或 `--init`）可以正确处理容器内的信号 — `docker-compose.yml` 中已配置
- FTB 是 TUI 应用，需要 `stdin_open: true` 和 `tty: true`（或 `-it`）才能正常运行 — `docker-compose.yml` 中已配置
- 如需在容器内使用 AI 功能或 SSH 连接，可以添加 `--network host` 共享宿主机网络栈
