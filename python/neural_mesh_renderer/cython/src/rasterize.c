#include "rasterize.h"

void cpp_forward_face_index_map(float* arr_a, int* arr_b, int size_a, int size_b)
{
    for(int i = 0;i < size_b;i++){
        arr_b[i] = 2;
    }
}