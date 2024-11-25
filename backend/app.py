from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import asyncio
import socket
import struct
import base64
import json

app = FastAPI()

# Set up a dictionary to store connections
connections = {}

async def send_message(websocket: WebSocket, message):
    await websocket.send_text(message)

async def receive_data(sock, buffer_size=1024):
    """
    Async function to receive data from a socket.
    This is an example; you can adapt it to your protocol.
    """
    # Receive command (4 bytes, for example)
    cmd_data = await asyncio.get_event_loop().sock_recv(sock, 2)
    if not cmd_data:
        return None, None, None

    cmd = int.from_bytes(cmd_data, "little")
    
    # Receive buffer size (4 bytes, for example)
    buff_size_data = await asyncio.get_event_loop().sock_recv(sock, 4)
    buff_size = int.from_bytes(buff_size_data, "little")

    # Receive the actual data
    data = b""
    while len(data) < buff_size:
        chunk = await asyncio.get_event_loop().sock_recv(sock, buff_size - len(data))
        if not chunk:
            break
        data += chunk
    
    return cmd, buff_size, data

async def keep_alive(websocket: WebSocket, interval=5):  # Send message every 10 minutes (600 seconds)
    try:
        # Connect the async socket
        if connections[websocket] is None:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setblocking(False)  # Set non-blocking mode for asyncio
            await asyncio.get_event_loop().sock_connect(sock, ("localhost", 8080))
            connections[websocket] = sock
        sock = connections[websocket]

        while True:
            # Periodically send keep-alive messages if needed
            # await asyncio.sleep(interval)

            # Receive data asynchronously from the socket
            cmd, buff_size, data = await receive_data(sock)

            if cmd is None:
                print("Socket closed by the server.")
                break

            if cmd == 1:
                pass  # Handle other command types
            elif cmd == 2:
                base64_encoded_image = base64.b64encode(data).decode("utf-8")
                # Send the Base64 encoded image data to the client
                await websocket.send_text(base64_encoded_image)

    except WebSocketDisconnect as e:
        print(f"Client disconnected: {e}")
    except Exception as e:
        print(f"Error in keep_alive: {e}")
    finally:
        if websocket in connections:
            connections.pop(websocket)
        sock.close()

@app.websocket("/")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    connections[websocket] = None

    async def keep_alive_thread():
        # try:
        await keep_alive(websocket)

    asyncio.create_task(keep_alive_thread())

    try:
        while True:
            data = await websocket.receive()
            data = json.loads(data["text"])
            session = connections[websocket]
            print(f"Received: {data}")
            await asyncio.get_event_loop().sock_sendall(session, struct.pack("h", data["command"]))
            await asyncio.get_event_loop().sock_sendall(session, struct.pack("i", 2))
            await asyncio.get_event_loop().sock_sendall(session, struct.pack("h", data["key"]))
            # Broadcast the message to all connected clients
            # await asyncio.gather(*[send_message(conn, data) for conn in connections])
    except WebSocketDisconnect as e:
        print(f"Client disconnected: {e}")
        connections.pop(websocket)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)