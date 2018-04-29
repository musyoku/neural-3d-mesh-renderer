import os
import numpy as np


def load_vertices(filepath):
    vertices = []
    with open(filepath) as file:
        for line in file:
            vertices.append([float(v) for v in line.split()])
    return np.vstack(vertices).astype("float32")


def load_faces(filepath):
    faces = []
    with open(filepath) as file:
        for line in file:
            faces.append([float(f) for f in line.split()])
    return np.vstack(faces).astype("int32")


def load_object(directory):
    vertices = load_vertices(os.path.join(directory, "vertices"))
    faces = load_faces(os.path.join(directory, "faces"))
    return vertices, faces