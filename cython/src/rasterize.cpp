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
                float yf = to_projected_coordinate((image_height - 1) - yi, image_height);
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

                    if (xi == 128) {
                        cout << xi << ", " << yi << " " << xf << ", " << yf << endl;
                    }

                    int map_index = batch_index * image_height * image_width + yi * image_width + xi;

                    /////////////////////
                    // depth_map[map_index] = 0.1;
                    // continue;
                    /////////////////////

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

// 点ABからなる辺の外側の画素を網羅
// *f_* \in [-1, 1]
// xi_* \in [0, image_width - 1]
// yi_* \in [0, image_height - 1]
void backward_outside_edge(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    int image_width,
    int image_height,
    int target_batch_index,
    int target_face_index,
    int* face_index_map,
    int* pixel_map,
    float* debug_grad_map)
{
    // 画像の座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
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

    // x軸を走査
    if (xi_a != xi_b) {
        // 投影座標系と画像座標系の変換で誤差がかなり出るので正確に計算する必要がある
        float pf_x_start = std::max(std::min(xf_a, xf_b), -1.0f);
        float pf_x_end = std::min(std::max(xf_a, xf_b), 1.0f);
        int pi_x_start = std::max((int)std::round(std::min(xi_a, xi_b)), 0);
        int pi_x_end = std::min((int)std::round(std::max(xi_a, xi_b)), image_width - 1);
        int length_p = pi_x_end - pi_x_start;
        float unit_x = 2.0f / image_width;
        float _f_x = pf_x_start - to_projected_coordinate(pi_x_start, image_width);
        float _i_x = _f_x / 2.0f * (image_width - 1);
        float __i_x = pi_x_start - to_image_coordinate(std::min(xf_a, xf_b), image_width);
        float _f_y_a = yf_a - to_projected_coordinate(yi_a, image_height);
        float _f_y_b = yf_b - to_projected_coordinate(yi_b, image_height);
        float _i_y_a = _f_y_a / 2.0f * (image_height - 1);
        float _i_y_b = _f_y_b / 2.0f * (image_height - 1);
        for (int pi_x = pi_x_start; pi_x <= pi_x_end; pi_x++) {
            // 辺上でx座標がpi_xの点を求める
            // 論文の図の点I_ijに相当（ここでは交点と呼ぶ）
            float di_x = pi_x - pi_x_start + __i_x;
            // 投影座標系と画像座標系は上下反転しているので傾きも反転する
            float pi_y = -(yf_a - yf_b) / (xf_a - xf_b) * di_x + ((xf_a < xf_b) ? yi_a : yi_b);
            // int pi_x = to_image_coordinate(pf_x, image_width);
            // int pi_y = to_image_coordinate(-pf_y, image_height);
            // 交点のy方向について、面の内側の一番辺に近い画素のy座標
            int pi_y_inside = (scan_direction == top_to_bottom) ? std::floor(pi_y + 0.5) : std::floor(pi_y - 0.5);

            if (pi_x == 128) {
                cout << "> " << pi_x << ", " << pi_y << " " << pi_y_inside << " " << scan_direction << endl;
            }
            // cout << pi_x << ", " << pi_y << ", " << pi_y_inside << ", " << yi_a << ", " << di_x << endl;
            // 交点のy方向について、面の外側（辺が通らない）の画素のy座標
            int pi_y_outside = (scan_direction == top_to_bottom) ? pi_y_inside - 1 : pi_y_inside + 1;
            if (pi_y_inside < 0 || pi_y_inside >= image_height) {
                continue;
            }
            if (pi_y_outside < 0 || pi_y_outside >= image_height) {
                continue;
            }

            int map_index_inside = target_batch_index * image_width * image_height + pi_y_inside * image_width + pi_x;
            int map_index_outside = target_batch_index * image_width * image_height + pi_y_outside * image_width + pi_x;
            int face_index = face_index_map[map_index_inside];

            
            int sd = pixel_map[map_index_inside];
            debug_grad_map[map_index_inside] = 255;
            if (face_index != target_face_index) {
                debug_grad_map[map_index_inside] = 64;
                continue;
            }
            int pixel_value_inside = pixel_map[map_index_inside];
        
            continue;

            // y軸を走査
            int si_y_start = (scan_direction == top_to_bottom) ? 0 : pi_y_outside;
            int si_y_end = (scan_direction == top_to_bottom) ? pi_y_outside : image_height - 1;
            for (int si_y = si_y_start; si_y <= si_y_end; si_y++) {
                int map_index_s = target_batch_index * image_width * image_height + si_y * image_width + pi_x;
                int pixel_value_s = pixel_map[map_index_s];
                debug_grad_map[map_index_s] = 255;

                // 内部は除く
                int face_index = face_index_map[map_index_s];
                if (face_index == target_face_index) {
                    continue;
                }
                // 走査点と面の輝度値の差
                float diff_pixel = pixel_value_inside - pixel_value_s;
                float grad = diff_pixel / std::abs(si_y - pi_y_inside);
            }
        }
    }
    // y方向の各画素を走査
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

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                continue;
            }

            // 3辺について
            backward_outside_edge(xf_1, yf_1, xf_2, yf_2, image_width, image_height, batch_index, face_index, face_index_map, pixel_map, debug_grad_map);
            backward_outside_edge(xf_2, yf_2, xf_3, yf_3, image_width, image_height, batch_index, face_index, face_index_map, pixel_map, debug_grad_map);
            backward_outside_edge(xf_3, yf_3, xf_1, yf_1, image_width, image_height, batch_index, face_index, face_index_map, pixel_map, debug_grad_map);
        }
    }
}