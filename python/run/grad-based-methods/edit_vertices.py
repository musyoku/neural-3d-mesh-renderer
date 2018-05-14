import os, sys, argparse
import math
import numpy as np
sys.path.append(os.path.join("..", ".."))
import neural_mesh_renderer as nmr


def main():
    # オブジェクトの読み込み
    vertices, faces = nmr.objects.load("../../objects/teapot.obj")

    # ミニバッチ化
    vertices_batch = vertices[None, ...]
    faces_batch = faces[None, ...]

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
            vertices_batch = nmr.vertices.rotate_x(
                vertices_batch, 1)
            vertices_batch = nmr.vertices.rotate_y(
                vertices_batch, 1)


            # カメラ座標系に変換
            perspective_vertices_batch = nmr.vertices.transform_to_camera_coordinate_system(
                vertices_batch,
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
            face_index_map = np.full(
                (batch_size, ) + silhouette_size, -1, dtype=np.int32)
            depth_map = np.zeros(
                (batch_size, ) + silhouette_size, dtype=np.float32)
            silhouette_image = np.zeros(
                (batch_size, ) + silhouette_size, dtype=np.int32)
            nmr.rasterizer.forward_face_index_map_cpu(
                face_vertices_batch, face_index_map, depth_map,
                silhouette_image)
            depth_map = np.ascontiguousarray(
                (1.0 - depth_map[0]) * 255).astype(np.uint8)
            #################

            #################
            grad_vertices = np.zeros_like(vertices_batch, dtype=np.float32)
            grad_silhouette = np.zeros_like(silhouette_image, dtype=np.float32)
            debug_grad_map = np.zeros_like(silhouette_image, dtype=np.float32)
            nmr.rasterizer.backward_silhouette_cpu(
                faces_batch, face_vertices_batch, vertices_batch,
                face_index_map, silhouette_image, grad_vertices,
                grad_silhouette, debug_grad_map)

            debug_grad_map /= np.amax(debug_grad_map)
            debug_grad_map *= 255

            vertices_batch -= 0.000001 * grad_vertices
            #################

            print(grad_vertices)

            browser.update_top_silhouette(depth_map)
            browser.update_bottom_silhouette(np.uint8(debug_grad_map[0]))
            browser.update_object(
                np.ascontiguousarray(vertices_batch[0]))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--use-browser", "-browser", action="store_true")
    args = parser.parse_args()
    main()