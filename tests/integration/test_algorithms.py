import pytest
import hashlib

@pytest.mark.parametrize("algo, hasher", [
    ("md5", hashlib.md5),
    ("sha256", hashlib.sha256),
    ("sha512", hashlib.sha512),
])
def test_algorithm_switching(run_server, client_factory, algo, hasher):
    host, port = run_server(algo=algo)
    client = client_factory(host, port)

    msg = "test_algorithm"
    client.send(msg + "\n")
    assert client.recv_line() == hasher(msg.encode()).hexdigest()