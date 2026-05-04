import pytest
import subprocess
import time
import socket
import os
import signal

# --- Utilities ---

def wait_for_port(host, port, timeout=2.0):
    """Wait until the server port becomes available."""
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            with socket.create_connection((host, port), timeout=0.1):
                return True
        except (ConnectionRefusedError, socket.timeout):
            time.sleep(0.1)
    return False

# --- Server Management ---

@pytest.fixture(scope="session")
def server_bin():
    path = os.path.abspath("build/hash_server")
    if not os.path.exists(path):
        pytest.fail(f"Server binary not found at {path}. Build the project first.")
    return path

@pytest.fixture
def run_server(server_bin):
    processes = []

    def _run(host="127.0.0.1", port=12345, algo="sha256", verbose=True):
        cmd = [server_bin, f"{host}:{port}", "-a", algo]
        if verbose:
            cmd.append("-v")

        # Use setsid to kill the whole process group later
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn=os.setsid
        )
        processes.append(proc)

        if not wait_for_port(host, port):
            proc.kill()
            pytest.fail(f"Server failed to bind to {host}:{port} within timeout.")

        return host, port

    yield _run

    for p in processes:
        if p.poll() is None:
            try:
                os.killpg(os.getpgid(p.pid), signal.SIGTERM)
                p.wait(timeout=2)
            except:
                os.killpg(os.getpgid(p.pid), signal.SIGKILL)

# --- Client Factory ---

class Client:
    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port), timeout=5)

    def send(self, data):
        if isinstance(data, str):
            data = data.encode()
        self.sock.sendall(data)

    def recv_line(self):
        buf = b""
        while not buf.endswith(b"\n"):
            chunk = self.sock.recv(1)
            if not chunk: break
            buf += chunk
        return buf.decode().strip()

    def close(self):
        self.sock.close()

@pytest.fixture
def client_factory():
    clients = []
    def _create(host, port):
        c = Client(host, port)
        clients.append(c)
        return c
    yield _create
    for c in clients:
        try: c.close()
        except: pass