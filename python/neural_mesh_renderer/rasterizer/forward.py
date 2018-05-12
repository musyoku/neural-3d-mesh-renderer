from . import rasterize_cpu


def forward_face_index_map_cpu(faces, face_index_map, depth_map):
    rasterize_cpu.forward_face_index_map(faces, face_index_map, depth_map)