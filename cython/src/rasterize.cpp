#include "rasterize.h"
#include <cmath>
#include <iostream>

using std::cout;
using std::endl;

float to_projected_coordinate(int p, int size)
{
    return 2.0 * (p / (float)(size - 1) - 0.5);
}

// [-1, 1] -> [0, size - 1]
float to_image_coordinate(float p, int size)
{
    return std::min(std::max(((p + 1.0f) * 0.5f * (size - 1)), 0.0f), (float)(size - 1));
}

// 各画素ごとに最前面を特定する
void cpp_forward_face_index_map(
    float* face_vertices,
    int* face_index_map,
    float* depth_map,
    int* silhouette_image,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height)
{
    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        // 初期化
        for (int yi = 0; yi < image_height; yi++) {
            for (int xi = 0; xi < image_width; xi++) {
                int index = batch_index * image_width * image_height + yi * image_width + xi;
                depth_map[index] = 1.0; // 最も遠い位置に初期化
            }
        }
        for (int face_index = 0; face_index < num_faces; face_index++) {
            int fv_index = batch_index * num_faces * 9 + face_index * 9;
            float xf_1 = face_vertices[fv_index + 0];
            float yf_1 = face_vertices[fv_index + 1];
            float zf_1 = face_vertices[fv_index + 2];
            float xf_2 = face_vertices[fv_index + 3];
            float yf_2 = face_vertices[fv_index + 4];
            float zf_2 = face_vertices[fv_index + 5];
            float xf_3 = face_vertices[fv_index + 6];
            float yf_3 = face_vertices[fv_index + 7];
            float zf_3 = face_vertices[fv_index + 8];

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                continue;
            }

            // 全画素についてループ
            for (int yi = 0; yi < image_height; yi++) {
                // yi \in [0, image_height] -> yf \in [-1, 1]
                float yf = -to_projected_coordinate(yi, image_height);
                // y座標が面の外部ならスキップ
                if ((yf > yf_1 && yf > yf_2 && yf > yf_3) || (yf < yf_1 && yf < yf_2 && yf < yf_3)) {
                    continue;
                }
                for (int xi = 0; xi < image_width; xi++) {
                    // xi \in [0, image_height] -> xf \in [-1, 1]
                    float xf = to_projected_coordinate(xi, image_width);

                    // xyが面の外部ならスキップ
                    // Edge Functionで3辺のいずれかの右側にあればスキップ
                    // https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
                    if ((yf - yf_1) * (xf_2 - xf_1) < (xf - xf_1) * (yf_2 - yf_1) || (yf - yf_2) * (xf_3 - xf_2) < (xf - xf_2) * (yf_3 - yf_2) || (yf - yf_3) * (xf_1 - xf_3) < (xf - xf_3) * (yf_1 - yf_3)) {
                        continue;
                    }

                    int map_index = batch_index * image_height * image_width + yi * image_width + xi;

                    // 重心座標系の各係数を計算
                    // http://zellij.hatenablog.com/entry/20131207/p1
                    float lambda_1 = ((yf_2 - yf_3) * (xf - xf_3) + (xf_3 - xf_2) * (yf - yf_3)) / ((yf_2 - yf_3) * (xf_1 - xf_3) + (xf_3 - xf_2) * (yf_1 - yf_3));
                    float lambda_2 = ((yf_3 - yf_1) * (xf - xf_3) + (xf_1 - xf_3) * (yf - yf_3)) / ((yf_2 - yf_3) * (xf_1 - xf_3) + (xf_3 - xf_2) * (yf_1 - yf_3));
                    float lambda_3 = 1.0 - lambda_1 - lambda_2;

                    // 面f_nのxy座標に対応する点のz座標を求める
                    // https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation
                    float z_face = 1.0 / (lambda_1 / zf_1 + lambda_2 / zf_2 + lambda_3 / zf_3);

                    // cout << "x = " << xi << ", y = " << yi << ", l1 = " << lambda_1 << ", l2 = " << lambda_2 << ", l3 = " << lambda_3 << ", z_face = " << z_face << endl;
                    if (z_face < 0.0 || z_face > 1.0) {
                        continue;
                    }
                    // zは小さい方が手前
                    float current_min_z = depth_map[map_index];
                    if (z_face < current_min_z) {
                        // 現在の面の方が前面の場合
                        depth_map[map_index] = z_face;
                        face_index_map[map_index] = face_index;
                        silhouette_image[map_index] = 255;
                    }
                }
            }
        }
    }
}

void compute_grad_outside_edge_y(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
{
    // 画像の座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    float xi_a = to_image_coordinate(xf_a, image_width);
    float xi_b = to_image_coordinate(xf_b, image_width);
    float yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    float yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);

    // y方向の走査がどの方向を向いていると辺に当たるか
    // 1:  画像の上から下に進む（yが増加する）方向に進んだ時に辺に当たる
    // -1: 画像の下から上に進む（yが減少する）方向に進んだ時に辺に当たる
    int top_to_bottom = 1;
    int bottom_to_top = -1;
    int scan_direction = (xi_a < xi_b) ? bottom_to_top : top_to_bottom;

    // 辺に沿ってx軸を走査
    int pi_x_start = std::max((int)std::round(std::min(xi_a, xi_b)), 0);
    int pi_x_end = std::min((int)std::round(std::max(xi_a, xi_b)), image_width - 1);
    // 辺上でx座標がpi_xの点を求める
    // 論文の図の点I_ijに相当（ここでは交点と呼ぶ）
    for (int pi_x = pi_x_start; pi_x <= pi_x_end; pi_x++) {
        // 辺に当たるまでy軸を走査
        // ここではスキャンラインと呼ぶことにする
        if (scan_direction == top_to_bottom) {
            // まずスキャンライン上で辺に当たる画素を探す
            int si_y_start = 0;
            int si_y_end = image_height - 1; // 実際にはここに到達する前に辺に当たるはず
            int si_y_edge = si_y_start;
            int pixel_value_inside = 0;
            for (int si_y = si_y_start; si_y <= si_y_end; si_y++) {
                int map_index_s = target_batch_index * image_width * image_height + si_y * image_width + pi_x;
                int face_index = face_index_map[map_index_s];
                if (face_index == target_face_index) {
                    si_y_edge = si_y;
                    pixel_value_inside = pixel_map[map_index_s];
                    break;
                }
            }
            for (int si_y = si_y_start; si_y < si_y_edge; si_y++) {
                int map_index_s = target_batch_index * image_width * image_height + si_y * image_width + pi_x;
                int pixel_value_outside = pixel_map[map_index_s];
                // 走査点と面の輝度値の差
                float diff_pixel = pixel_value_inside - pixel_value_outside;
                // 頂点の実際の移動量を求める
                // スキャンライン上の移動距離ではない
                // 相似な三角形なのでx方向の比率から求まる
                // 頂点Aについて
                {
                    if (pi_x - pi_x_start > 0) {
                        float moving_distance = (si_y_edge - si_y) / (float)(pi_x - pi_x_start) * (float)(pi_x_end - pi_x_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + si_y * image_width + pi_x];
                            float grad = diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
                // 頂点Bについて
                {
                    if (pi_x_end - pi_x > 0) {
                        float moving_distance = (si_y_edge - si_y) / (float)(pi_x_end - pi_x) * (float)(pi_x_end - pi_x_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + si_y * image_width + pi_x];
                            float grad = diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
            }
        } else {
            int si_y_start = image_height - 1;
            int si_y_end = 0;
            int si_y_edge = si_y_start;
            int pixel_value_inside = 0;
            for (int si_y = si_y_start; si_y >= si_y_end; si_y--) {
                int map_index_s = target_batch_index * image_width * image_height + si_y * image_width + pi_x;
                int face_index = face_index_map[map_index_s];
                if (face_index == target_face_index) {
                    si_y_edge = si_y;
                    pixel_value_inside = pixel_map[map_index_s];
                    break;
                }
            }
            for (int si_y = si_y_start; si_y > si_y_edge; si_y--) {
                int map_index_s = target_batch_index * image_width * image_height + si_y * image_width + pi_x;
                int pixel_value_outside = pixel_map[map_index_s];
                float diff_pixel = pixel_value_inside - pixel_value_outside;

                // 頂点Aについて
                {
                    if (pi_x - pi_x_start > 0) {
                        float moving_distance = (si_y - si_y_edge) / (float)(pi_x - pi_x_start) * (float)(pi_x_end - pi_x_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + si_y * image_width + pi_x];
                            float grad = -diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
                // 頂点Bについて
                {
                    if (pi_x_end - pi_x > 0) {
                        float moving_distance = (si_y - si_y_edge) / (float)(pi_x_end - pi_x) * (float)(pi_x_end - pi_x_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + si_y * image_width + pi_x];
                            float grad = -diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
            }
        }
    }
    // y方向の各画素を走査
}

void compute_grad_outside_edge_x(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
{
    // 画像の座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    float xi_a = to_image_coordinate(xf_a, image_width);
    float xi_b = to_image_coordinate(xf_b, image_width);
    float yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    float yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);

    // x方向の走査がどの方向を向いていると辺に当たるか
    // 1:  画像の左から右に進む（xが増加する）方向に進んだ時に辺に当たる
    // -1: 画像の右から左に進む（xが減少する）方向に進んだ時に辺に当たる
    int left_to_right = 1;
    int right_to_left = -1;
    int scan_direction = (yi_a < yi_b) ? left_to_right : right_to_left;

    // 辺に沿ってy軸を走査
    int pi_y_start = std::max((int)std::round(std::min(yi_a, yi_b)), 0);
    int pi_y_end = std::min((int)std::round(std::max(yi_a, yi_b)), image_height - 1);
    // 辺上でy座標がpi_yの点を求める
    // 論文の図の点I_ijに相当（ここでは交点と呼ぶ）
    for (int pi_y = pi_y_start; pi_y <= pi_y_end; pi_y++) {
        // 辺に当たるまでx軸を走査
        // ここではスキャンラインと呼ぶことにする
        if (scan_direction == left_to_right) {
            // まずスキャンライン上で辺に当たる画素を探す
            int si_x_start = 0;
            int si_x_end = image_width - 1; // 実際にはここに到達する前に辺に当たるはず
            int si_x_edge = si_x_start;
            int pixel_value_inside = 0;
            for (int si_x = si_x_start; si_x <= si_x_end; si_x++) {
                int map_index_s = target_batch_index * image_width * image_height + pi_y * image_width + si_x;
                int face_index = face_index_map[map_index_s];
                if (face_index == target_face_index) {
                    si_x_edge = si_x;
                    pixel_value_inside = pixel_map[map_index_s];
                    break;
                }
            }
            for (int si_x = si_x_start; si_x < si_x_edge; si_x++) {
                int map_index_s = target_batch_index * image_width * image_height + pi_y * image_width + si_x;
                int pixel_value_outside = pixel_map[map_index_s];
                // 走査点と面の輝度値の差
                float diff_pixel = pixel_value_inside - pixel_value_outside;
                // 頂点の実際の移動量を求める
                // スキャンライン上の移動距離ではない
                // 相似な三角形なのでy方向の比率から求まる
                // 頂点Aについて
                {
                    if (pi_y - pi_y_start > 0) {
                        float moving_distance = (si_x_edge - si_x) * (float)(pi_y - pi_y_start) / (float)(pi_y_end - pi_y_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + pi_y * image_width + si_x];
                            float grad = -diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
                // 頂点Bについて
                {
                    if (pi_y_end - pi_y > 0) {
                        float moving_distance = (si_x_edge - si_x) * (float)(pi_y_end - pi_y) / (float)(pi_y_end - pi_y_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + pi_y * image_width + si_x];
                            float grad = -diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
            }
        } else {
            int si_x_start = image_width - 1;
            int si_x_end = 0;
            int si_x_edge = si_x_start;
            int pixel_value_inside = 0;
            for (int si_x = si_x_start; si_x >= si_x_end; si_x--) {
                int map_index_s = target_batch_index * image_width * image_height + pi_y * image_width + si_x;
                int face_index = face_index_map[map_index_s];
                if (face_index == target_face_index) {
                    si_x_edge = si_x;
                    pixel_value_inside = pixel_map[map_index_s];
                    break;
                }
            }
            for (int si_x = si_x_start; si_x > si_x_edge; si_x--) {
                int map_index_s = target_batch_index * image_width * image_height + pi_y * image_width + si_x;
                int pixel_value_outside = pixel_map[map_index_s];
                float diff_pixel = pixel_value_inside - pixel_value_outside;

                // 頂点Aについて
                {
                    if (pi_y - pi_y_start > 0) {
                        float moving_distance = (si_x - si_x_edge) * (float)(pi_y - pi_y_start) / (float)(pi_y_end - pi_y_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + pi_y * image_width + si_x];
                            float grad = diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
                // 頂点Bについて
                {
                    if (pi_y_end - pi_y > 0) {
                        float moving_distance = (si_x - si_x_edge) * (float)(pi_y_end - pi_y) / (float)(pi_y_end - pi_y_start);
                        if (moving_distance > 0) {
                            float propagated_grad = grad_silhouette[target_batch_index * image_width * image_height + pi_y * image_width + si_x];
                            float grad = diff_pixel / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad * propagated_grad;
                            debug_grad_map[map_index_s] += grad * propagated_grad;
                        }
                    }
                }
            }
        }
    }
    // y方向の各画素を走査
}

// 点ABからなる辺の外側の画素を網羅
// *f_* \in [-1, 1]
// xi_* \in [0, image_width - 1]
// yi_* \in [0, image_height - 1]
void compute_grad_outside_edge(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
{
    compute_grad_outside_edge_x(xf_a, yf_a, xf_b, yf_b, vertex_index_a, vertex_index_b, image_width, image_height, num_vertices, target_batch_index, target_face_index, face_index_map, pixel_map, grad_vertices, grad_silhouette, debug_grad_map);
    compute_grad_outside_edge_y(xf_a, yf_a, xf_b, yf_b, vertex_index_a, vertex_index_b, image_width, image_height, num_vertices, target_batch_index, target_face_index, face_index_map, pixel_map, grad_vertices, grad_silhouette, debug_grad_map);
}
// シルエットの誤差から各頂点の勾配を求める
void cpp_backward_silhouette(
    int* faces,
    float* face_vertices,
    float* vertices,
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map,
    int batch_size,
    int num_faces,
    int num_vertices,
    int image_width,
    int image_height)
{
    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        for (int face_index = 0; face_index < num_faces; face_index++) {
            int fv_index = batch_index * num_faces * 9 + face_index * 9;
            float xf_1 = face_vertices[fv_index + 0];
            float yf_1 = face_vertices[fv_index + 1];
            // float zf_1 = face_vertices[fv_index + 2];
            float xf_2 = face_vertices[fv_index + 3];
            float yf_2 = face_vertices[fv_index + 4];
            // float zf_2 = face_vertices[fv_index + 5];
            float xf_3 = face_vertices[fv_index + 6];
            float yf_3 = face_vertices[fv_index + 7];
            // float zf_3 = face_vertices[fv_index + 8];

            int v_index = batch_index * num_faces * 3 + face_index * 3;
            int vertex_1_index = faces[v_index + 0];
            int vertex_2_index = faces[v_index + 1];
            int vertex_3_index = faces[v_index + 2];

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                continue;
            }

            // 3辺について
            compute_grad_outside_edge(xf_1, yf_1, xf_2, yf_2, vertex_1_index, vertex_2_index, image_width, image_height, num_vertices, batch_index, face_index, face_index_map, pixel_map, grad_vertices, grad_silhouette, debug_grad_map);
            compute_grad_outside_edge(xf_2, yf_2, xf_3, yf_3, vertex_2_index, vertex_3_index, image_width, image_height, num_vertices, batch_index, face_index, face_index_map, pixel_map, grad_vertices, grad_silhouette, debug_grad_map);
            compute_grad_outside_edge(xf_3, yf_3, xf_1, yf_1, vertex_3_index, vertex_1_index, image_width, image_height, num_vertices, batch_index, face_index, face_index_map, pixel_map, grad_vertices, grad_silhouette, debug_grad_map);
        }
    }
}