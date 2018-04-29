import os
import numpy as np


def save_vertices(filepath, vertices):
    with open(filepath, "w") as file:
        lines = []
        for vertex in vertices:
            lines.append("{} {} {}".format(vertex[0], vertex[1], vertex[2]))
        file.write("\n".join(lines))


def save_faces(filepath, faces):
    with open(filepath, "w") as file:
        lines = []
        for face in faces:
            lines.append("{} {} {}".format(face[0], face[1], face[2]))
        file.write("\n".join(lines))


def save_object(directory, vertices, faces):
    try:
        os.mkdir(directory)
    except:
        pass
    save_vertices(os.path.join(directory, "vertices"), vertices)
    save_faces(os.path.join(directory, "faces"), faces)