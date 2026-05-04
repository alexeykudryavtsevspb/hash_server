import pytest
import threading
import hashlib

@pytest.mark.slow
def test_high_concurrency(run_server, client_factory):
    host, port = run_server()
    client_count = 100
    results = [None] * client_count

    def client_thread(idx):
        try:
            client = client_factory(host, port)
            msg = f"payload_{idx}"
            client.send(msg + "\n")
            results[idx] = (client.recv_line() == hashlib.sha256(msg.encode()).hexdigest())
        except Exception:
            results[idx] = False

    threads = [threading.Thread(target=client_thread, args=(i,)) for i in range(client_count)]
    for t in threads: t.start()
    for t in threads: t.join()

    assert all(results)

@pytest.mark.slow
def test_large_payload(run_server, client_factory):
    host, port = run_server()
    client = client_factory(host, port)

    # 1MB string
    large_str = "a" * (1024 * 1024)
    client.send(large_str + "\n")

    expected = hashlib.sha256(large_str.encode()).hexdigest()
    assert client.recv_line() == expected