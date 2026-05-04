BUILD_DIR = build
VENV = .venv
PYTHON = $(VENV)/bin/python3
PIP = $(VENV)/bin/pip
IMAGE_NAME = hash-server
TAG = latest
TYPE ?= Release

.PHONY: all build clean format utest venv itest_fast itest_smoke itest_stress \
        itest_all all_test docker_build docker_run docker_stop benchmark

all: venv format build test

build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(TYPE) .. && cmake --build . -j$(shell nproc)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(VENV)
	@rm -rf .pytest_cache
	@rm -rf tests/integration/__pycache__
	@echo "Cleanup complete."

format:
	@echo "Formatting source files..."
	@clang-format -i $(shell find src tests -name '*.cpp' -o -name '*.h')

utest: build
	@echo "Running unit tests..."
	@cd $(BUILD_DIR) && ctest --output-on-failure

send_hello:
	echo "hello" | nc -N localhost 12345

### Integration tests

venv: $(VENV)/bin/activate

$(VENV)/bin/activate: requirements.txt
	@echo "Creating virtual environment..."
	@python3 -m venv $(VENV)
	@$(PIP) install --upgrade pip
	@$(PIP) install -r requirements.txt
	@touch $(VENV)/bin/activate

itest_fast: venv build
	@echo "Running fast integration tests..."
	@$(PYTHON) -m pytest -m "not slow" tests/integration/

itest_all: venv build
	@echo "Running all integration tests..."
	@$(PYTHON) -m pytest tests/integration/

all_test: utest itest_all


### Docker

docker_build:
	@echo "--- Starting Docker build ---"
	docker build -t $(IMAGE_NAME):$(TAG) .

docker_run:
	@echo "--- Running Docker container ---"
	docker run --rm -it -p 12345:12345 --name $(IMAGE_NAME) $(IMAGE_NAME):$(TAG)

docker_run_bg:
	@echo "--- Running Docker container in background ---"
	docker run -d -p 12345:12345 --name $(IMAGE_NAME) $(IMAGE_NAME):$(TAG)
	@echo "Server is running. Use 'make docker_stop' to stop it."

docker_status:
	docker ps -f name=$(IMAGE_NAME)

docker_logs:
	docker logs -f $(IMAGE_NAME)

docker_stop:
	@echo "--- Stopping Docker container ---"
	docker stop $(IMAGE_NAME)
	docker rm $(IMAGE_NAME)

docker_clean:
	@echo "--- Removing Docker image ---"
	docker rmi $(IMAGE_NAME):$(TAG)


### Benchmark

BENCH_HOST = 0.0.0.0
BENCH_PORT = 12345
BENCH_ALGO = sha256

benchmark: build
	@echo "Starting benchmark environment..."
	@./build/hash_server -v $(BENCH_HOST):$(BENCH_PORT) --algo $(BENCH_ALGO) > /dev/null 2>&1 & \
	SERVER_PID=$$!; \
	echo "Server started with PID $$SERVER_PID"; \
	sleep 1; \
	./build/server_benchmark; \
	EXIT_STATUS=$$?; \
	echo "Cleaning up server (PID $$SERVER_PID)..."; \
	kill $$SERVER_PID 2>/dev/null || true; \
	wait $$SERVER_PID 2>/dev/null || true; \
	exit $$EXIT_STATUS
