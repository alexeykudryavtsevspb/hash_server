import pytest
import socket
import time
import hashlib

def test_client_disconnect_mid_stream(run_server):
    """
    Test that the server handles a client disconnecting
    without finishing the message.
    """
    host, port = run_server()

    sock = socket.create_connection((host, port), timeout=2)
    # Send partial data and close immediately
    sock.sendall(b"partial message without newline")
    sock.close()

    # Give the server a moment to log or clean up
    time.sleep(0.1)

    # Try to connect again to ensure the server is still alive
    sock2 = socket.create_connection((host, port), timeout=2)
    sock2.sendall(b"new_start\n")
    response = sock2.recv(1024).decode().strip()
    assert response == hashlib.sha256(b"new_start").hexdigest()

def test_malformed_input_no_newline(run_server, client_factory):
    """
    Test that the server doesn't respond until a newline is received,
    and handles very long inputs without crashing.
    """
    host, port = run_server()
    client = client_factory(host, port)

    # Send data without newline
    client.send("half_data")

    # Ensure no response yet
    client.sock.settimeout(0.5)
    with pytest.raises(socket.timeout):
        client.sock.recv(1024)

    # Send the rest
    client.send("_complete\n")
    expected = hashlib.sha256(b"half_data_complete").hexdigest()
    assert client.recv_line() == expected

def test_empty_lines(run_server, client_factory):
    """
    Check if the server handles multiple consecutive newlines correctly.
    """
    host, port = run_server()
    client = client_factory(host, port)

    # Send several empty lines
    client.send("\n\n\n")

    empty_hash = hashlib.sha256(b"").hexdigest()
    assert client.recv_line() == empty_hash
    assert client.recv_line() == empty_hash
    assert client.recv_line() == empty_hash

def test_binary_garbage_input(run_server, client_factory):
    """
    Send non-UTF8 binary data. The server should still
    calculate the hash correctly as it treats data as a byte stream.
    """
    host, port = run_server()
    client = client_factory(host, port)

    # Random binary bytes followed by newline
    garbage = b"\xff\x00\x12\xfe\n"
    client.sock.sendall(garbage)

    expected = hashlib.sha256(garbage[:-1]).hexdigest()
    assert client.recv_line() == expected