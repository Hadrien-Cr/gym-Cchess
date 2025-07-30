from setuptools import setup, Extension
import os
import pybind11

# Define the path to the C++ source files
sources = [
    'gym_chessengine/binding.cpp',
    'gym_chessengine/nnue/misc.cpp',
    'gym_chessengine/nnue/nnue.cpp',
]

# Define the extension module
env_extension = Extension(
    'gym_chessengine.binding',
    sources=sources,
    include_dirs=['gym_chessengine', 'gym_chessengine/nnue', pybind11.get_include()],
    language='c++', # Specify C++ language
    extra_compile_args=['-std=c++17'], # Use C++17 standard
)

setup(
    name='gym_chessengine',
    version='1.0',
    description='A fast C++ chess engine, wrapped in a Gym Environment.',
    author='Hadrien Crassous',
    author_email='crassous.hadrien@gmail.com',
    packages=['gym_chessengine'],
    ext_modules=[env_extension],
    install_requires=[
        'pybind11>=2.10',
        'gymnasium>=1.2.0',
    ],
    # Include the .nnue file as package data
    package_data={
        'gym_chessengine': ['nn-eba324f53044.nnue'],
    },
    include_package_data=True,
)
