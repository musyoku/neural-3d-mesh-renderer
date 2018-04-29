import websocket


class Client:
    def __init__(self, domain, port):
        websocket.enableTrace(True)
        self.ws = websocket.WebSocketApp(
            "ws://{}:{}".format(domain, port),
            on_message=self.on_message,
            on_error=self.on_error,
            on_close=self.on_close)

        self.ws.on_open = self.on_open
        self.ws.run_forever()

    def update_object(self, vertices, faces):
        print(vertices)
        print(faces)
        self.ws.send("Hello!")

    def on_message(self, ws, message):
        print(message)

    def on_error(self, ws, error):
        print(error)

    def on_close(self, ws):
        print("closed connection")

    def on_open(self, ws):
        print("connected websocket server")
