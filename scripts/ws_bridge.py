import asyncio
import websockets
import socket
import json

connected_clients = set()

async def register(websocket):
    connected_clients.add(websocket)
    print("[BRIDGE] React Dashboard Connected!")
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.remove(websocket)
        print("[BRIDGE] React Dashboard Disconnected")

async def poll_cpp_engine():
    """Connects to C++ Telemetry Port 8083 and broadcasts to all connected React clients."""
    while True:
        try:
            reader, writer = await asyncio.open_connection('127.0.0.1', 8083)
            print("[BRIDGE] Connected to C++ Engine Telemetry Feed (Port 8083)")
            while True:
                data = await reader.readline()
                if not data:
                    break
                
                # Broadcast payload to React frontend
                if connected_clients:
                    message = data.decode('utf-8').strip()
                    websockets.broadcast(connected_clients, message)
        except (ConnectionRefusedError, OSError):
            # Engine not ready yet, retry in 1 second
            await asyncio.sleep(1)

async def main():
    # Change 8082 to 8084 here
    async with websockets.serve(register, "localhost", 8084):
        print("[BRIDGE] WebSocket Server running on ws://localhost:8084")
        await poll_cpp_engine()

if __name__ == "__main__":
    asyncio.run(main())