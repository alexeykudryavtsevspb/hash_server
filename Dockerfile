# --- Stage 1: Build ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libboost-system-dev \
    libboost-thread-dev \
    libssl-dev \
    libgtest-dev \
    libspdlog-dev \
    python3 \
    python3-venv \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make build && make all_test

# --- Stage 2: Runtime ---
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    libspdlog1.12 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/hash_server .

EXPOSE 12345

ENTRYPOINT ["./hash_server"]
CMD ["0.0.0.0:12345", "--algo", "sha256"]
