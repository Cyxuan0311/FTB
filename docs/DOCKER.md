[中文文档](DOCKER_CN.md)  |  English

# Docker

FTB provides a Docker setup for running in an isolated environment without installing dependencies on the host system. The Dockerfile uses a multi-stage build to keep the final image small (~33 MB content size).

## Prerequisites

- [Docker](https://docs.docker.com/engine/install/) (24+)
- [Docker Compose](https://docs.docker.com/compose/install/) (v2+, included with Docker Desktop)

## Quick Start

```bash
# Build and run with Docker Compose
docker compose build
docker compose run ftb /usr/local/bin/FTB /mnt/host
```

## Build

### Direct build (requires GitHub access)

```bash
docker compose build
```

### Build with mirror (recommended in China)

```bash
docker build \
  --build-arg GIT_MIRROR=https://gh-proxy.com/https://github.com \
  -t ftb .
```

The `GIT_MIRROR` argument changes where FTXUI's source code is cloned from. It defaults to `https://github.com`.

## Run

### With Docker Compose

```bash
# Enter bash shell
docker compose run ftb bash

# Launch FTB directly
docker compose run ftb /usr/local/bin/FTB /mnt/host

# Launch FTB in current directory
docker compose run ftb /usr/local/bin/FTB $(pwd)
```

### With Docker

```bash
docker run -it --rm \
  -v ~/.ftb:/root/.ftb \
  -v ~/:/mnt/host \
  ftb

# With custom directory
docker run -it --rm \
  -v ~/.ftb:/root/.ftb \
  -v $(pwd):/mnt/host \
  ftb \
  /usr/local/bin/FTB /mnt/host
```

## Volume Mounts

| Host Path | Container Path | Purpose |
|-----------|---------------|---------|
| `~/.ftb` | `/root/.ftb` | Persist FTB configuration |
| `~/` | `/mnt/host` | Access host files from inside container |

## Build Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `GIT_MIRROR` | `https://github.com` | Git mirror URL for cloning FTXUI source |

## Dockerfile Structure

The build consists of three stages:

1. **ftxui-builder** — Compiles FTXUI v6.0.0 from source
2. **builder** — Compiles FTB and links against the built FTXUI
3. **runtime** — Minimal image containing only the FTB binary and runtime dependencies (tini)

## Tips

- Use `init: true` (or `--init`) to handle signals correctly inside the container — already configured in `docker-compose.yml`
- FTB is a TUI application and requires `stdin_open: true` and `tty: true` (or `-it`) to function — already configured in `docker-compose.yml`
- To use AI features or SSH connections from inside the container, use `--network host` to share the host network stack
