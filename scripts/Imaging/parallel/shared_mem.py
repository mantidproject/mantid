from __future__ import (absolute_import, division, print_function)
import numpy as np
import multiprocessing

from recon.helper import Helper

# this global is necessary for the child processes to access the original
# array and overwrite the values in-place
shared_data = None


def inplace_forward_func(func, i, **kwargs):
    """
    Forward a function that WILL NOT return some data, and will just overwrite it inplace.
    Use if the parameter function does NOT have a return statement, and thus will not return any data.

    You HAVE to be careful when using this, for example the func:
    def _apply_normalise_inplace(data, dark=None, norm_divide=None, clip_min=None, clip_max=None):
        data = np.clip(np.true_divide(
            data - dark, norm_divide), clip_min, clip_max)

    DOES NOT CHANGE THE DATA! Because the data = ... variable inside is just a local variable that is discarded.

    The proper way to write this function is:
    def _apply_normalise_inplace(data, dark=None, norm_divide=None, clip_min=None, clip_max=None):
        data[:] = np.clip(np.true_divide(
            data - dark, norm_divide), clip_min, clip_max)

    Notice the data[:], what this does is refer to the ACTUAL parameter, and then changes it's contents, as [:] gives
    a reference back to the inner contents.

    :param func: Function that will be executed
    :param i: index from the shared_data on which to operate
    :param kwargs: kwargs to forward to the function func that will be executed
    :return: nothing is returned, as the data is replaced in place
    """
    func(shared_data[i], **kwargs)


def forward_func(func, i, **kwargs):
    """
    Forward a function that WILL RETURN some data, and will not just overwrite it inplace.
    Use if the parameter function DOES have a return statement, and thus will return some data back.

    If a function seems to give back unexpected Nones or nans, then it might not be returning anything,
    and is doing all the calculations and overwriting in place. In that case use forward_func_inplace
    as a forward_function parameter for create_partial, creating something like:

    f = parallel.create_partial(func_to_be_executed, parallel.inplace_forward_func, **kwargs)

    :param func: Function that will be executed
    :param i: index from the shared_data on which to operate
    :param kwargs: kwargs to forward to the function func that will be executed
    :return: nothing is returned, as the data is replaced by assigning the return value from the func
    """
    shared_data[i] = func(shared_data[i], **kwargs)


def create_partial(func, forward_function=forward_func, **kwargs):
    """
    Create a partial using functools.partial, to forward the kwargs to the parallel execution of imap.

    :param func: Function that will be executed
    :param forward_function: The function will be forwarded through function. It must be one of:
            - shared_parallel.forward_func: if the function returns a value
            - shared_parallel.inplace_forward_func: if the function will overwrite the data in place
    :param kwargs: kwargs to forward to the function func that will be executed
    :return:
    """
    from functools import partial
    return partial(forward_function, func, **kwargs)


def execute(data=None, partial_func=None, cores=1, chunksize=None, name="Progress", h=None, output_data=None, show_timer=True):
    """
    Executes a function in parallel with shared memory between the processes.
    The array must have been created using parallel.create_shared_array(shape, dtype).

    - imap_unordered gives the images back in random order!
    - map and map_async cannot replace the data in place and end up
    doubling the memory. They do not improve speed performance either
    - imap seems to be the best choice

    Using _ in the for _ enumerate is slightly faster, because the tuple
    from enumerate isn't unpacked, and thus some time is saved.

    From performance tests, the chunksize doesn't seem to make much of a difference,
    but having larger chunks usually led to slower performance:

    Shape: (50,512,512)
    1 chunk 3.06s
    2 chunks 3.05s
    3 chunks 3.07s
    4 chunks 3.06s
    5 chunks 3.16s
    6 chunks 3.06s
    7 chunks 3.058s
    8 chunks 3.25s
    9 chunks 3.45s

    :param data: the data array that will be processed in parallel
    :param partial_func: a function constructed using partial to pass the correct arguments
    :param cores: number of cores that the processing will use
    :param chunksize: chunk of work per process(worker)
    :param name: the string that will be appended in front of the progress bar
    :param h: ...
    :param output_data:
    :return:
    """
    h = Helper.empty_init() if h is None else h

    if chunksize is None:
        chunksize = 1  # TODO use proper calculation

    # handle the edge case of having a different output that input i.e. rebin,
    # crop, etc
    if output_data is None:
        # get data reference to original with [:]
        output_data = data[:]

    global shared_data
    # get reference to output data
    # if different shape it will get the reference to the new array
    shared_data = output_data[:]

    from multiprocessing import Pool
    pool = Pool(cores)
    img_num = shared_data.shape[0]
    if show_timer:
        h.prog_init(img_num, name + " " + str(cores) +
                    "c " + str(chunksize) + "chs")

    indices_list = [i for i in range(img_num)]
    for _ in enumerate(pool.imap(partial_func, indices_list, chunksize=chunksize)):
        h.prog_update()

    pool.close()
    pool.join()
    h.prog_close()

    return shared_data


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
