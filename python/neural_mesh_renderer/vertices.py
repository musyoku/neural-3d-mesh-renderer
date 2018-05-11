import math, chainer


def angle_to_radian(angle):
    return angle / 180.0 * math.pi


# 各面の各頂点番号に対応する座標を取る
def convert_to_face_representation(vertices, faces):
    assert (vertices.ndim == 3)
    assert (faces.ndim == 3)
    assert (vertices.shape[0] == faces.shape[0])
    assert (vertices.shape[2] == 3)
    assert (faces.shape[2] == 3)

    xp = chainer.cuda.get_array_module(faces)
    batch_size, num_vertices = vertices.shape[:2]
    faces = faces + (
        xp.arange(batch_size, dtype=xp.int32) * num_vertices)[:, None, None]
    vertices = vertices.reshape((batch_size * num_vertices, 3)).astype(
        xp.float32)
    return vertices[faces]


# 透視変換
# https://qiita.com/ryutorion/items/0824a8d6f27564e850c9
# http://satoh.cs.uec.ac.jp/ja/lecture/ComputerGraphics/3.pdf
def project_perspective(vertices, viewing_angle, z_max=5, z_min=1, d=1):
    assert (vertices.ndim == 3)
    assert (vertices.shape[2] == 3)

    xp = chainer.cuda.get_array_module(vertices)
    z = vertices[..., None, 2]
    z_a = (z_max + z_min) / (z_max - z_min)
    z_b = 2.0 * z_max * z_min / (z_max - z_min)
    viewing_rad_half = angle_to_radian(viewing_angle / 2.0)
    projection_mat = xp.asarray([[1.0 / math.tan(viewing_rad_half), 0, 0],
                                 [0.0, 1.0 / math.tan(viewing_rad_half),
                                  0], [0, 0, z_a]])
    vertices = xp.dot(vertices, projection_mat)
    vertices += xp.asarray([[0, 0, z_b]])
    vertices /= z

    ##########
    vertices /= xp.asarray([[1, 1, z_max]])
    ##########

    return vertices.astype(xp.float32)


# カメラ座標系への変換
# 右手座標系
# カメラから見える範囲にあるオブジェクトのz座標は全て負になる
def transform_to_camera_coordinate_system(vertices, distance_z, angle_x,
                                          angle_y):
    assert (vertices.ndim == 3)
    assert (vertices.shape[2] == 3)

    xp = chainer.cuda.get_array_module(vertices)
    rad_x = angle_to_radian(angle_x)
    rad_y = angle_to_radian(angle_y)
    # rad_z = angle_to_radian(180.0)
    rotation_mat_x = xp.asarray([
        [1, 0, 0],
        [0, math.cos(rad_x), -math.sin(rad_x)],
        [0, math.sin(rad_x), math.cos(rad_x)],
    ])
    rotation_mat_y = xp.asarray([
        [math.cos(rad_y), 0, math.sin(rad_y)],
        [0, 1, 0],
        [-math.sin(rad_y), 0, math.cos(rad_y)],
    ])
    # rotation_mat_z = xp.asarray([
    #     [math.cos(rad_z), -math.sin(rad_z), 0],
    #     [math.sin(rad_z), math.cos(rad_z), 0],
    #     [0, 0, 1],
    # ])
    vertices = xp.dot(vertices, rotation_mat_x.T)
    vertices = xp.dot(vertices, rotation_mat_y.T)
    vertices += xp.asarray([[0, 0, -distance_z]])
    # vertices = xp.dot(vertices, rotation_mat_z.T)
    return vertices.astype(xp.float32)