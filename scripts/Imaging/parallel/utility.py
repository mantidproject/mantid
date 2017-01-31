from __future__ import (absolute_import, division, print_function)

import multiprocessing
import numpy as np


def create_shared_array(shape, dtype=np.float32):
    import ctypes

    ctype = ctypes.c_float  # default to numpy float32 / C type float
    if isinstance(dtype, np.uint16) or dtype == 'uint16':
        ctype = ctypes.c_int16
        dtype = np.uint16
    elif isinstance(dtype, np.float32) or dtype == 'float32':
        ctype = ctypes.c_float
        dtype = np.float32
    elif isinstance(dtype, np.float64) or dtype == 'float64':
        ctype = ctypes.c_double
        dtype = np.float64

    shared_array_base = multiprocessing.Array(
        ctype, shape[0] * shape[1] * shape[2])

    # create a numpy array from shared array
    data = np.frombuffer(shared_array_base.get_obj(), dtype=dtype)
    return data.reshape(shape)


def multiprocessing_available():
    try:
        import multiprocessing
        return True
    except ImportError:
        return False


def generate_indices(num_images):
    return xrange(num_images)


def calculate_chunksize(cores):
    # TODO possible proper calculation of chunksize, although best performance has been with 1
    return 1
