import websocket

websocket.enableTrace(True)
ws = websocket.WebSocket()
ws.connect("ws://192.168.30.116/ws")
ws.send("Hello, Server")
print(ws.recv())
while True:
  line = input("> ")
  if line == "quit":
    break
  ws.send(line)
  print(ws.recv())
ws.close()