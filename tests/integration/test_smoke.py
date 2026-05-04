import pytest
import hashlib
import signal
import os

def test_smoke_server_lifecycle(run_server, client_factory, server_bin):
    """
    Validates basic connectivity, hashing, and graceful shutdown via SIGINT.
    """
    # 1. Setup: Start server and create client
    host, port = run_server(verbose=True)
    client = client_factory(host, port)

    # 2. Functional Check: Verify SHA-256 for "hello"
    msg = "hello"
    expected_hash = hashlib.sha256(msg.encode()).hexdigest()

    client.send(msg + "\n")
    response = client.recv_line()

    assert response == expected_hash, f"Smoke test failed: Expected {expected_hash}, got {response}"

    # 3. Graceful Shutdown Check:
    # Since the 'run_server' fixture manages the process, we need to find the process
    # to test if it handles SIGINT correctly as the original smoke test did.
    # Note: In a standard pytest run, the fixture handles cleanup,
    # but here we explicitly test the SIGINT behavior.

    import subprocess
    import time

    # Start a separate instance to specifically test the signal return code
    proc = subprocess.Popen(
        [server_bin, f"{host}:{port+1}"],
        preexec_fn=os.setsid
    )
    time.sleep(0.3) # Wait for startup

    # Send SIGINT (Ctrl+C)
    os.killpg(os.getpgid(proc.pid), signal.SIGINT)

    # Wait for process to exit and verify exit code
    ret_code = proc.wait(timeout=5)
    assert ret_code == 0, f"Server did not exit gracefully. Exit code: {ret_code}"