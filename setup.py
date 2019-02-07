#!/usr/bin/env python
import os
import sys

from setuptools import setup, Extension

WIN32 = sys.platform.startswith("win")

sources = [
    'src/python/core.c',
    'src/libethash/io.c',
    'src/libethash/internal.c',
    'src/libethash/sha3.c']
if os.name == 'nt':
    sources += [
        'src/libethash/util_win32.c',
        'src/libethash/io_win32.c',
        'src/libethash/mmap_win32.c',
    ]
else:
    sources += [
        'src/libethash/io_posix.c'
    ]
depends = [
    'src/libethash/ethash.h',
    'src/libethash/compiler.h',
    'src/libethash/data_sizes.h',
    'src/libethash/endian.h',
    'src/libethash/ethash.h',
    'src/libethash/io.h',
    'src/libethash/fnv.h',
    'src/libethash/internal.h',
    'src/libethash/sha3.h',
    'src/libethash/util.h',
]

ccargs=[
    "/Isrc/",
    "/Wall"
] if WIN32 else [
    "-Isrc/",
    "-std=gnu99",
    "-Wall"
]

ldargs=["shell32.lib"] if WIN32 else []

pyethash = Extension('pyethash',
                     sources=sources,
                     depends=depends,
                     extra_compile_args=ccargs,
                     extra_link_args=ldargs)

setup(
    name='pyethash',
    author="Matthew Wampler-Doty",
    author_email="matthew.wampler.doty@gmail.com",
    license='GPL',
    version='0.1.27',
    url='https://github.com/ethereum/ethash',
    download_url='https://github.com/ethereum/ethash/tarball/v23',
    description=('Python wrappers for ethash, the ethereum proof of work'
                 'hashing function'),
    ext_modules=[pyethash],
)
