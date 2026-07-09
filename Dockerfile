# ============================================================
# Stage 1: Build FTXUI from source
# ============================================================
FROM debian:bookworm-slim AS ftxui-builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

ARG GIT_MIRROR=https://github.com

RUN apt-get update && apt-get install -y --no-install-recommends git && rm -rf /var/lib/apt/lists/*

WORKDIR /ftxui
RUN git clone --depth 1 --branch v6.0.0 ${GIT_MIRROR}/ArthurSonzogni/FTXUI.git .
RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/ftxui/install \
        -DFTXUI_BUILD_DOCS=OFF \
        -DFTXUI_BUILD_EXAMPLES=OFF \
        -DFTXUI_BUILD_TESTS=OFF \
    && cmake --build build -j"$(nproc)" \
    && cmake --install build

# ============================================================
# Stage 2: Build FTB
# ============================================================
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    g++ \
    make \
    nlohmann-json3-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=ftxui-builder /ftxui/install /usr/local

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DFTB_ENABLE_ICONS=ON \
    -DFTB_ENABLE_PLUGINS=ON \
    && cmake --build build -j"$(nproc)" \
    && cmake --install build --prefix /install

# ============================================================
# Stage 3: Runtime
# ============================================================
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    tini \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /install /usr/local

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/usr/local/bin/FTB"]