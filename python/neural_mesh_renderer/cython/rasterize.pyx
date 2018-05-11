import cython
import numpy as np
cimport numpy as np

cdef extern from "src/rasterize.h":
    void cpp_forward_face_index_map(
        float *faces, 
        int *face_index_map, 
        float *depth_map, 
        int batch_size,
        int num_faces,
        int image_width,
        int Fimage_height)

@cython.boundscheck(False)
@cython.wraparound(False)
def forward_face_index_map(
        np.ndarray[float, ndim=4, mode="c"] faces not None, 
        np.ndarray[int, ndim=3, mode="c"] face_index_map not None,
        np.ndarray[float, ndim=3, mode="c"] depth_map not None):
    batch_size, num_faces = faces.shape[:2]
    image_width, image_height = face_index_map.shape[1:3]
    assert(faces.shape[2] == 3)
    assert(faces.shape[3] == 3)
    assert(face_index_map.shape[1] == depth_map.shape[1])
    assert(face_index_map.shape[2] == depth_map.shape[2])

    return cpp_forward_face_index_map(
        &faces[0, 0, 0, 0], 
        &face_index_map[0, 0, 0],
        &depth_map[0, 0, 0], 
        batch_size,
        num_faces,
        image_width,
        image_height)