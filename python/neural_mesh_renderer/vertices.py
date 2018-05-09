import math
import numpy as np


def angle_to_radian(angle):
    return angle / 180.0 * math.pi


# 透視変換
# https://qiita.com/ryutorion/items/0824a8d6f27564e850c9
# http://satoh.cs.uec.ac.jp/ja/lecture/ComputerGraphics/3.pdf
def project_perspective(vertices,
                        viewing_angle_x,
                        viewing_angle_y,
                        z_max=10,
                        z_min=1,
                        d=1):
    z = vertices[:, None, 2]
    z_a = (z_max + z_min) / (z_max - z_min)
    z_b = 2.0 * z_max * z_min / (z_max - z_min)
    viewing_rad_x_half = angle_to_radian(viewing_angle_x / 2.0)
    viewing_rad_y_half = angle_to_radian(viewing_angle_y / 2.0)
    projection_mat = np.asarray([[1.0 / math.tan(viewing_rad_x_half), 0, 0],
                                 [0.0, 1.0 / math.tan(viewing_rad_y_half),
                                  0], [0, 0, z_a]])
    vertices = np.dot(vertices, projection_mat)
    vertices += np.asarray([[0, 0, z_b]])
    vertices /= -z
    return vertices


# カメラ座標系への変換
# 右手座標系
# カメラから見える範囲にあるオブジェクトのz座標は全て負になる
def transform_to_camera_coordinate_system(vertices, distance_z, angle_x,
                                          angle_y):
    rad_x = angle_to_radian(angle_x)
    rad_y = angle_to_radian(angle_y)
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
    vertices += np.asarray([[0, 0, -distance_z]])
    return vertices