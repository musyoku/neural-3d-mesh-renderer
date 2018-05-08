import os, sys, argparse
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.load_object("../objects/teapot.obj")

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        brwoser = nmr.brwoser.Client("localhost", 8080)
        brwoser.init_object(vertices, faces)


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--use-browser", "-browser", action="store_true")
	args = parser.parse_args()
    main()
