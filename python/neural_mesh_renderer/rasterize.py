import chainer

class Rasterize(chainer.Function):
    def __init__(self, image_size, z_min, z_max):
        self.image_size = image_size
        self.z_min = z_min
        self.z_max = z_max

    def forward_face_index_map_gpu(self):
        pass

    def forward_face_index_map_cpu(self):
        pass

    def forward_gpu(self, inputs):
        pass

    def forward_cpu(self, inputs):
        pass