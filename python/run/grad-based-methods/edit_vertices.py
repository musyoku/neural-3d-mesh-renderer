import os, sys, argparse
import numpy as np
sys.path.append(os.path.join("..", ".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../../objects/cube.obj")

    # ミニバッチ化
    vertices_batch = vertices[None, :]
    faces_batch = faces[None, :]

    # カメラ座標系に変換
    vertices_batch = nmr.vertices.transform_to_camera_coordinate_system(
        vertices_batch, 5, 0, 0)

    face_vertices_batch = nmr.vertices.convert_to_face_representation(vertices_batch, faces_batch)

    silhouette_size = (256, 256)

    vertices_batch = nmr.vertices.project_perspective(vertices_batch, 45)
    image_data = nmr.image.draw_vertices(vertices_batch[0], silhouette_size)

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        browser = nmr.browser.Silhouette(8080,
                                         np.ascontiguousarray(
                                             vertices_batch[0]),
                                         np.ascontiguousarray(faces_batch[0]),
                                         silhouette_size)

        for angle in range(1, 180):
            perspective_vertices = nmr.vertices.project_perspective(
                vertices_batch, angle)
            silhouette_data = nmr.image.draw_vertices(perspective_vertices[0],
                                                      silhouette_size)
            browser.update_top_silhouette(silhouette_data)
            browser.update_object(
                np.ascontiguousarray(perspective_vertices[0]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()