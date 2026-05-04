import pytest
import hashlib

def get_expected(text: str):
    return hashlib.sha256(text.encode()).hexdigest()

def test_single_request(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)

    msg = "hello"
    client.send(msg + "\n")
    assert client.recv_line() == get_expected(msg)

def test_multiple_lines(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)

    lines = ["one", "two", "three"]
    for line in lines:
        client.send(line + "\n")
        assert client.recv_line() == get_expected(line)

def test_partial_send(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)

    # Send "hello\n" in small chunks
    msg = "chunked_message\n"
    for char in msg:
        client.send(char)

    assert client.recv_line() == get_expected("chunked_message")