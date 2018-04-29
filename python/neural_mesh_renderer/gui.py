import requests
import struct


class Client:
    def __init__(self, domain, port):
        self.base_url = "http://{}:{}".format(domain, port)
        print(self.base_url)

    def pack(self, vertices, faces):
        # 先頭に頂点数を入れ、続けて座標を入れる
        return struct.pack("=i{}f".format(vertices.size), vertices.shape[0],
                           *vertices.flatten("C"))

    def init_object(self, vertices, faces):
        data = self.pack(vertices, faces)
        requests.post(
            url="{}/init_object".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

    def update_object(self, vertices, faces):
        data = self.pack(vertices, faces)
        print(vertices.shape)
        requests.post(
            url="{}/update_object".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

    def on_message(self, ws, message):
        print(message)

    def on_error(self, ws, error):
        print(error)

    def on_close(self, ws):
        print("closed connection")

    def on_open(self, ws):
        print("connected websocket server")
