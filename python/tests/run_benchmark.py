import os, sys
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr
import math
import numpy as np
import sys


def main():
    vertices, faces = nmr.objects.load("../objects/bunny.obj")
    browser = nmr.browser.Silhouette("localhost", 8080)
    browser.init_object(vertices, faces)

    import time
    start_time = time.time()
    for i in range(10000):
        rad = float(i % 360) / 180.0 * math.pi
        rotation = np.array([
            [math.cos(rad), -math.sin(rad), 0],
            [math.sin(rad), math.cos(rad), 0],
            [0, 0, 1],
        ])
        noise_vertices = np.dot(vertices, rotation.T) + np.random.normal(
            0, 0.02, size=vertices.shape)
        browser.update_object(noise_vertices)
        elasped_time = time.time() - start_time
        sys.stdout.write("\r{}FPS".format(int(i / elasped_time)))
        sys.stdout.flush()


if __name__ == "__main__":
    main()
