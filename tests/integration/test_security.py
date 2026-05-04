import pytest
import time
import socket
import hashlib

@pytest.mark.slow
def test_idle_timeout(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)
    time.sleep(31)
    data = client.sock.recv(1024)
    assert data == b""

@pytest.mark.slow
def test_slow_loris_protection_reset(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)
    chunks = ["slow_", "data_", "test"]
    for chunk in chunks:
        client.send(chunk)
        time.sleep(10)
    client.send("\n")
    expected = hashlib.sha256(b"slow_data_test").hexdigest()
    assert client.recv_line() == expected

def test_binary_garbage_stream(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)
    garbage = b"\xff\x00\x12\xfe\xca\xfe\n"
    client.sock.sendall(garbage)
    expected = hashlib.sha256(garbage[:-1]).hexdigest()
    assert client.recv_line() == expected

def test_multiple_empty_lines_dos(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)
    count = 100
    client.send("\n" * count)
    expected_empty_hash = hashlib.sha256(b"").hexdigest()
    for _ in range(count):
        assert client.recv_line() == expected_empty_hash

@pytest.mark.slow
def test_very_long_string_state(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)
    chunk = "s" * (1024 * 1024)
    total_data = b""
    for _ in range(5):
        client.send(chunk)
        total_data += chunk.encode()
        time.sleep(0.1)
    client.send("\n")
    expected = hashlib.sha256(total_data).hexdigest()
    assert client.recv_line() == expected

def test_client_abrupt_disconnect(run_server):
    host, port = run_server()
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    sock.sendall(b"partial message")
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b'\x01\x00\x00\x00\x00\x00\x00\x00')
    sock.close()
    time.sleep(0.2)
    sock2 = socket.create_connection((host, port), timeout=1)
    sock2.sendall(b"ping\n")
    assert hashlib.sha256(b"ping").hexdigest() in sock2.recv(1024).decode()