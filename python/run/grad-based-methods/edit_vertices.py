import os, sys, argparse
import numpy as np
sys.path.append(os.path.join("..", ".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../../objects/bunny.obj")

    # ミニバッチ化
    vertices_batch = vertices[None, :]
    faces_batch = faces[None, :]

    # カメラ座標系に変換
    # vertices_batch = nmr.vertices.transform_to_camera_coordinate_system(
    #     vertices_batch, 5, 0, 0)

    silhouette_size = (256, 256)

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        perspective_vertices_batch = nmr.vertices.project_perspective(
            vertices_batch, 30)
        browser = nmr.browser.Silhouette(8080,
                                         np.ascontiguousarray(
                                             vertices_batch[0]),
                                         np.ascontiguousarray(faces_batch[0]),
                                         silhouette_size)

        for loop in range(10000):

            #########
            import math
            rad_x = math.pi * (loop % 360) / 180.0
            rotation_mat_x = np.asarray([
                [1, 0, 0],
                [0, math.cos(rad_x), -math.sin(rad_x)],
                [0, math.sin(rad_x), math.cos(rad_x)],
            ])
            # print(vertices_batch.shape, rotation_mat_x.shape)
            rotated_vertices_batch = np.dot(
                vertices_batch, rotation_mat_x.T) + np.random.normal(
                    0, 0.02, size=vertices_batch.shape)
            perspective_vertices_batch = rotated_vertices_batch + np.asarray(
                [[0, 0, -3]], dtype=np.float32)
            perspective_vertices_batch /= np.asarray(
                [[2, 2, -5]], dtype=np.float32)
            #########

            # perspective_vertices_batch = nmr.vertices.project_perspective(
            #     vertices_batch, 100)

            #################
            face_vertices_batch = nmr.vertices.convert_to_face_representation(
                perspective_vertices_batch, faces_batch)
            # print(face_vertices_batch.shape)
            batch_size = face_vertices_batch.shape[0]
            face_index_map = np.zeros(
                (batch_size, ) + silhouette_size, dtype=np.int32)
            depth_map = np.zeros(
                (batch_size, ) + silhouette_size, dtype=np.float32)
            nmr.rasterize.forward_face_index_map_cpu(face_vertices_batch,
                                                     face_index_map, depth_map)
            # print(perspective_vertices_batch)
            # print(face_index_map)
            depth_map = np.ascontiguousarray(
                (1.0 - depth_map[0]) * 255).astype(np.uint8)
            # print(depth_map)
            #################

            silhouette_data = nmr.image.draw_vertices(
                perspective_vertices_batch[0], silhouette_size)
            browser.update_top_silhouette(depth_map)
            browser.update_object(
                np.ascontiguousarray(rotated_vertices_batch[0]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()