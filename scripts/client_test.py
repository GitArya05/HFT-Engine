import socket
import struct

# TCP Server details
HOST = '127.0.0.1'
PORT = 8080

# Binary packet layout matching RawNetworkOrder: id (uint64), price (uint64), qty (uint32), is_buy (uint8)
# Let's send a valid order: ID 9999, Price 50000, Qty 200, Is Buy = 1
order_id = 9999
price = 50000
qty = 200
is_buy = 1

packet = struct.pack('<QQQI', order_id, price, qty, is_buy)

print(f"[CLIENT] Connecting to HFT Gateway at {HOST}:{PORT}...")
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print("[CLIENT] Sending binary order payload...")
    s.sendall(packet)
    response = s.recv(1024)
    print(f"[CLIENT] Server response: {response.decode().strip()}")