from PIL import Image, ImageDraw, ImageFont, ImageMath
import numpy as np


def draw_vertices(vertices, size, point_radius=1):
    image_width, image_height = size
    image = Image.new("L", size)
    draw = ImageDraw.Draw(image)
    for v in vertices:
        x = (v[0] + 1) / 2.0 * image_width
        y = (1.0 - (v[1] + 1) / 2.0) * image_height
        draw.ellipse(
            (x - point_radius, y - point_radius, x + point_radius,
             y + point_radius),
            fill="white")
    return np.array(image)