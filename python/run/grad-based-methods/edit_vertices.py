import os, sys, argparse
import math
import numpy as np
sys.path.append(os.path.join("..", ".."))
import neural_mesh_renderer as nmr


def rotate(vertices, angle):
    rad_x = math.pi * (angle % 360) / 180.0
    rad_y = math.pi * (angle % 360) / 180.0
    rotation_mat_x = np.asarray([
        [1, 0, 0],
        [0, math.cos(rad_x), -math.sin(rad_x)],
        [0, math.sin(rad_x), math.cos(rad_x)],
    ])
    rotation_mat_y = np.asarray([
        [math.cos(rad_y), 0, math.sin(rad_y)],
        [0, 1, 0],
        [-math.sin(rad_y), 0, math.cos(rad_y)],
    ])
    vertices = np.dot(vertices, rotation_mat_x.T)
    vertices = np.dot(vertices, rotation_mat_y.T)
    return vertices


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../../objects/teapot.obj")

    print(np.amax(vertices, axis=0), np.amin(vertices, axis=0))

    # ミニバッチ化
    vertices_batch = vertices[None, :]
    faces_batch = faces[None, :]

    silhouette_size = (256, 256)

    if args.use_browser:
        # ブラウザのビューワを起動
        # nodeのサーバーをあらかじめ起動しておかないと繋がらない
        browser = nmr.browser.Silhouette(8080,
                                         np.ascontiguousarray(
                                             vertices_batch[0]),
                                         np.ascontiguousarray(faces_batch[0]),
                                         silhouette_size)

        for loop in range(10000):
            # 回転
            modified_vertices_batch = rotate(vertices_batch, loop % 360)

            # ノイズ
            modified_vertices_batch += np.random.normal(
                0, 0.02, size=vertices.shape)

            # カメラ座標系に変換
            perspective_vertices_batch = nmr.vertices.transform_to_camera_coordinate_system(
                modified_vertices_batch,
                distance_from_object=2,
                angle_x=0,
                angle_y=0)

            # 透視投影
            perspective_vertices_batch = nmr.vertices.project_perspective(
                perspective_vertices_batch, viewing_angle=45, z_max=5, z_min=0)

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
                np.ascontiguousarray(modified_vertices_batch[0]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()