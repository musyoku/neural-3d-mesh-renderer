import cython
import numpy as np
cimport numpy as np

cdef extern from "src/rasterize.h":
    void cpp_forward_face_index_map(float *arr_a, int *arr_b, int size_a, int size_b)

@cython.boundscheck(False)
@cython.wraparound(False)
def forward_face_index_map(
    np.ndarray[float, ndim=3, mode="c"] faces not None, 
    np.ndarray[int, ndim=3, mode="c"] face_index_map not None):
    return cpp_forward_face_index_map(&faces[0, 0, 0], &face_index_map[0, 0, 0], len(faces), len(face_index_map))