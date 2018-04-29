import os, sys
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    vertices, faces = nmr.load_object("../objects/teapot.obj")
    gui = nmr.gui.Client("localhost", 8080)
    gui.init_object(vertices, faces)

    from time import sleep
    for i in range(100):
        vertices -= [-0.1, 0, 0]
        gui.update_object(vertices)
        sleep(1)


if __name__ == "__main__":
    main()
