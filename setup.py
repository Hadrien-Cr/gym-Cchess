from setuptools import setup, Extension
import pybind11

cpp_args = ['-std=c++11']

ext_modules = [
    Extension(
        'myenv_cpp',
        ['env.cpp', 'nnue/nnue.cpp', 'nnue/misc.cpp'],
        include_dirs=[pybind11.get_include()],
        language='c++',
        extra_compile_args=cpp_args,
    ),
]

setup(
    name='myenv_cpp',
    version='1.0',
    description='Python wrapper for C++ chess environment',
    ext_modules=ext_modules,
)
