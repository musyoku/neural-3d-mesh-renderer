from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
from Cython.Build import cythonize
import numpy

base_dir = "python/neural_mesh_renderer/cython"
setup(
    ext_modules=cythonize(
        Extension(
            "rasterize_cpu",
            [
                "{}/rasterize.pyx".format(base_dir),
                "{}/src/rasterize.c".format(base_dir)
            ],
            extra_compile_args=["-O3"],
            language="c",
            include_dirs=[numpy.get_include()],
        ),
        build_dir="build"),
    cmdclass={"build_ext": build_ext},
)