#include "rasterize.h"

void cpp_forward_face_index_map(
    double* faces,
    int* face_index_map,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height)
{
    for (int h = 0; h < image_height; h++) {
        for (int w = 0; w < image_width; w++) {
            int index = h * image_width + w;
            face_index_map[index] = h;
        }
    }
}