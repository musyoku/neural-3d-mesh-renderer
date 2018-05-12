import requests
import struct


class Silhouette:
    def __init__(self, port, vertices, faces, silhouette_size):
        self.base_url = "http://localhost:{}/silhouette".format(port)
        print(self.base_url)
        self.init_object(vertices, faces)
        self.init_silhouette_area(silhouette_size)

    def pack(self, vertices, faces):
        return struct.pack("=i{}f".format(vertices.size), vertices.shape[0],
                           *vertices.flatten("C"))

    def init_silhouette_area(self, silhouette_size):
        data = struct.pack("<2i", silhouette_size[0], silhouette_size[1])
        requests.post(
            url="{}/init_silhouette_area".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

    def update_top_silhouette(self, image):
        assert (len(image.shape) == 2)
        data = struct.pack(
            "<3i{}B".format(image.size),
            image.size,
            image.shape[0],  # 高さ
            image.shape[1],  # 幅
            *image.flatten("C"),
        )
        requests.post(
            url="{}/update_top_silhouette".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

    def update_bottom_silhouette(self, image):
        assert (len(image.shape) == 2)
        data = struct.pack(
            "<3i{}B".format(image.size),
            image.size,
            image.shape[0],  # 高さ
            image.shape[1],  # 幅
            *image.flatten("C"),
        )
        requests.post(
            url="{}/update_bottom_silhouette".format(self.base_url),
            data=data,
            headers={'Content-Type': 'application/octet-stream'})

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
