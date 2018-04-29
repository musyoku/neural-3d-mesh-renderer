import os
import numpy as np


def load_vertices(filepath):
    vertices = []
    with open(filepath) as file:
        for line in file:
            vertices.append([float(v) for v in line.split()])
    return np.vstack(vertices).astype(np.float32)


def load_faces(filepath):
    faces = []
    with open(filepath) as file:
        for line in file:
            faces.append([float(f) for f in line.split()])
    # 番号なので1を引いてインデックスに直す
    return np.vstack(faces).astype(np.int32) - 1


def load_object(directory):
    vertices = load_vertices(os.path.join(directory, "vertices"))
    faces = load_faces(os.path.join(directory, "faces"))
    return vertices, faces