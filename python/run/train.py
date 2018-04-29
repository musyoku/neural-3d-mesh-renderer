import os, sys
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    vertices, faces = nmr.load_object("../objects/teapot.obj")
    gui = nmr.gui.Client("localhost", 8080)
    gui.init_object(vertices, faces)

if __name__ == "__main__":
    main()
