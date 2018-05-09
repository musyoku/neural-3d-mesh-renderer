import os, sys, argparse
sys.path.append(os.path.join(".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../objects/teapot.obj")

    # カメラ座標系に変換
    vertices = nmr.vertices.transform_to_camera_coordinate_system(
        vertices, 5, 0, 0)
    print(vertices)

    silhouette_size = (256, 256)

    vertices = nmr.vertices.project_perspective(vertices, 45)
    image_data = nmr.image.draw_vertices(vertices, silhouette_size)
    print(image_data.shape)

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        browser = nmr.browser.Silhouette(8080, vertices, faces, silhouette_size)

        for i in range(1, 180):
            perspective_vertices = nmr.vertices.project_perspective(vertices, i)
            silhouette_data = nmr.image.draw_vertices(perspective_vertices, silhouette_size)
            print(silhouette_data.shape)
            browser.update_top_silhouette(silhouette_data)
            browser.update_object(perspective_vertices)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()