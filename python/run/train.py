import os, sys, argparse
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../objects/bunny.obj")

    # カメラ座標系に変換
    vertices = nmr.vertices.transform_to_camera_coordinate_system(
        vertices, 5, 0, 0)
    print(vertices)

    vertices = nmr.vertices.project_perspective(vertices, 45)
    print(vertices)

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        browser = nmr.browser.Silhouette("localhost", 8080)
        browser.init_object(vertices, faces)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()