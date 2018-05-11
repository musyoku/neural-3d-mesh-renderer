#include "rasterize.h"
#include <iostream>

using std::cout;
using std::endl;

// 各画素ごとに最前面を特定する
void cpp_forward_face_index_map(
    float* face_vertices,
    int* face_index_map,
    float* depth_map,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height)
{
    for (int bn = 0; bn < batch_size; bn++) {
        // 初期化
        for (int yi = 0; yi < image_height; yi++) {
            for (int xi = 0; xi < image_width; xi++) {
                int index = bn * image_width * image_height + yi * image_width + xi;
                depth_map[index] = 1.0; // 最も遠い位置に初期化
            }
        }
        for (int fn = 0; fn < num_faces; fn++) {
            int face_index = bn * num_faces * 9 + fn * 9;
            float x_1 = face_vertices[face_index + 0];
            float y_1 = face_vertices[face_index + 1];
            float z_1 = face_vertices[face_index + 2];
            float x_2 = face_vertices[face_index + 3];
            float y_2 = face_vertices[face_index + 4];
            float z_2 = face_vertices[face_index + 5];
            float x_3 = face_vertices[face_index + 6];
            float y_3 = face_vertices[face_index + 7];
            float z_3 = face_vertices[face_index + 8];

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((y_1 - y_3) * (x_1 - x_2) < (y_1 - y_2) * (x_1 - x_3)) {
                continue;
            }

            // 全画素についてループ
            for (int yi = 0; yi < image_height; yi++) {
                // yi \in [0, image_height] -> yp \in [-1, 1]
                float yp = 2.0 * (yi / (float)image_height - 0.5);
                // y座標が面の外部ならスキップ
                if ((yp > y_1 && yp > y_2 && yp > y_3) || (yp < y_1 && yp < y_2 && yp < y_3)) {
                    continue;
                }
                for (int xi = 0; xi < image_width; xi++) {
                    // xi \in [0, image_height] -> xp \in [-1, 1]
                    float xp = 2.0 * (xi / (float)image_width - 0.5);

                    // xyが面の外部ならスキップ
                    // Edge Functionで3辺のいずれかの右側にあればスキップ
                    // https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
                    if ((yp - y_1) * (x_2 - x_1) < (xp - x_1) * (y_2 - y_1) || (yp - y_2) * (x_3 - x_2) < (xp - x_2) * (y_3 - y_2) || (yp - y_3) * (x_1 - x_3) < (xp - x_3) * (y_1 - y_3)) {
                        continue;
                    }


                    int map_index = bn * image_height * image_width + yi * image_width + xi;

                    /////////////////////
                    // depth_map[map_index] = 0.1;
                    // continue;
                    /////////////////////


                    // 重心座標系の各係数を計算
                    // http://zellij.hatenablog.com/entry/20131207/p1
                    float lambda_1 = ((y_2 - y_3) * (xp - x_3) + (x_3 - x_2) * (yp - y_3)) / ((y_2 - y_3) * (x_1 - x_3) + (x_3 - x_2) * (y_1 - y_3));
                    float lambda_2 = ((y_3 - y_1) * (xp - x_3) + (x_1 - x_3) * (yp - y_3)) / ((y_2 - y_3) * (x_1 - x_3) + (x_3 - x_2) * (y_1 - y_3));
                    float lambda_3 = 1.0 - lambda_1 - lambda_2;

                    // 面f_nのxy座標に対応する点のz座標を求める
                    // https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation
                    float z_fn = 1.0 / (lambda_1 / z_1 + lambda_2 / z_2 + lambda_3 / z_3);

                    // cout << "x = " << xi << ", y = " << yi << ", l1 = " << lambda_1 << ", l2 = " << lambda_2 << ", l3 = " << lambda_3 << ", z_fn = " << z_fn << endl;
                    if (z_fn < 0.0 || z_fn > 1.0) {
                        continue;
                    }
                    // zは小さい方が手前
                    float current_min_z = depth_map[map_index];
                    if (z_fn < current_min_z) {
                        // 現在の面の方が前面の場合
                        depth_map[map_index] = z_fn;
                        face_index_map[map_index] = fn;
                    }
                }
            }
        }
    }
}

// シルエットの誤差から各頂点の勾配を求める
void backward_silhouette(
    float* face_vertices,
    int* face_index_map,
    float* depth_map,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height)
{
}