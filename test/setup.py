from distutils.core import setup
from Cython.Build import cythonize

setup(
    ext_modules=cythonize(
        "tool.pyx",
        compiler_directives={'language_level': "3"},
        build_dir="build"
        # extra_compile_args=["-std=c11"],
    )
)
