from setuptools import Extension, setup

ext = Extension(
    name='core',
    sources=['./core.cpp'],
    define_macros=[('PY_SSIZE_T_CLEAN', None)],
    include_dirs=[],
    library_dirs=[],
    libraries=['gdi32', 'user32', 'opengl32', 'powrprof'],
)

setup(
    name='core',
    version='0.1.0',
    ext_modules=[ext],
)
