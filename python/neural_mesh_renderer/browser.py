import requests
import struct


class Client:
    def __init__(self, domain, port):
        self.base_url = "http://{}:{}".format(domain, port)
        print(self.base_url)

    def pack(self, vertices, faces):
        return struct.pack("=i{}f".format(vertices.size), vertices.shape[0],
                           *vertices.flatten("C"))

    def init_object(self, vertices, faces):
        # 先頭に頂点数を入れ、続けて座標を入れる
        # 次に面の数を入れ、続けて頂点インデックスを入れる
        data = struct.pack("<i{}fi{}i".format(vertices.size,
                                              faces.size), vertices.shape[0],
                           *vertices.flatten("C"), faces.shape[0],
                           *faces.flatten("C"))
        requests.post(
            url="{}/init_object".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

    def update_object(self, vertices):
        # 先頭に頂点数を入れ、続けて座標を入れる
        data = struct.pack("<i{}f".format(vertices.size), vertices.shape[0],
                           *vertices.flatten("C"))
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
