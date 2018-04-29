import os, sys
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    vertices, faces = nmr.load_object("../objects/teapot.obj")
    gui = nmr.gui.Client("localhost", 8081)
    from time import sleep
    while True:
        gui.update_object(vertices, faces)
        print("ha")

if __name__ == "__main__":
    main()
