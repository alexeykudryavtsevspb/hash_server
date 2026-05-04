import pytest
import socket
import threading
import time
import hashlib

@pytest.mark.slow
def test_hanging_connections(run_server, client_factory):
    """
    Test that 100 idle connections don't block the 101st active connection.
    This validates that io_context threads are not blocked by waiting for I/O.
    """
    host, port = run_server()
    hanging_count = 100
    idle_clients = []

    # 1. Open 100 connections and send data WITHOUT a newline
    try:
        for i in range(hanging_count):
            client = client_factory(host, port)
            # Send data without \n to keep the hash_engine state active but not finalized
            client.send(f"incomplete_data_{i}")
            idle_clients.append(client)

        # Give the server a small window to process all accept events
        time.sleep(0.5)

        # 2. Open the 101st connection and send a complete request
        active_client = client_factory(host, port)
        active_msg = "I_am_active"
        active_client.send(active_msg + "\n")

        # 3. Check if the active client gets a response immediately
        expected_hash = hashlib.sha256(active_msg.encode()).hexdigest()
        response = active_client.recv_line()

        assert response == expected_hash, "Active client should be served despite idle ones"

        # 4. Verify that idle clients still haven't received anything
        # We set a small timeout to not wait forever
        for client in idle_clients:
            client.sock.settimeout(0.1)
            try:
                data = client.sock.recv(1024)
                assert not data, "Idle client should not have received any hash yet"
            except socket.timeout:
                pass # This is expected

    finally:
        # Cleanup is handled by client_factory fixture in conftest.py
        pass

@pytest.mark.slow
def test_slow_client_recovery(run_server, client_factory):
    """
    Test that an idle client can eventually finish its request.
    """
    host, port = run_server()
    client = client_factory(host, port)

    # Send part of the message
    client.send("start_")
    time.sleep(1) # Simulated delay

    # Send the rest
    client.send("end\n")

    expected_hash = hashlib.sha256(b"start_end").hexdigest()
    assert client.recv_line() == expected_hash