from __future__ import (absolute_import, division, print_function)
import numpy as np
import multiprocessing

from recon.helper import Helper

# this global is necessary for the child processes to access the original
# array and overwrite the values in-place
shared_data = None
second_shared_data = None


def inplace_fwd_func(func, i, **kwargs):
    """
    Use if the parameter function does NOT have a return statement, and will overwrite the data in place.

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
    func(shared_data[i], second_shared_data[i], **kwargs)


def inplace_fwd_func_second_2d(func, i, **kwargs):
    """
    Use if the parameter function does NOT have a return statement, and will overwrite the data in place.
    Use to share a single 2D second_shared_data, i.e. a single averaged flat image.

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
    func(shared_data[i], second_shared_data, **kwargs)


def fwd_func_return_to_first(func, i, **kwargs):
    """
    Use if the parameter function does have a return statement. The result will be stored in the first shared_data.

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
    shared_data[i] = func(shared_data[i], second_shared_data[i], **kwargs)


def fwd_func_return_to_second(func, i, **kwargs):
    """
    Use if the parameter function does have a return statement. The result will be stored in the second shared_data.

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
    second_shared_data[i] = func(
        shared_data[i], second_shared_data[i], **kwargs)


def create_partial(func, fwd_function=inplace_fwd_func, **kwargs):
    """
    Create a partial using functools.partial, to forward the kwargs to the parallel execution of imap.

    :param func: Function that will be executed
    :param fwd_function: The function will be forwarded through function. It must be one of:
            - shared_parallel.fwd_func: if the function returns a value
            - shared_parallel.inplace_fwd_func: if the function will overwrite the data in place
    :param kwargs: kwargs to forward to the function func that will be executed
    :return:
    """
    from functools import partial
    return partial(fwd_function, func, **kwargs)


def execute(data=None, second_data=None, partial_func=None, cores=8, chunksize=None, name="Progress", h=None, show_timer=True):
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

    :param data: the shared data array that will be processed in parallel
    :param second_data: the second shared data array that will be processed in parallel
    :param partial_func: a function constructed using partial to pass the correct arguments
    :param cores: number of cores that the processing will use
    :param chunksize: chunk of work per process(worker)
    :param name: the string that will be appended in front of the progress bar
    :param h: the helper class
    :return:
    """
    h = Helper.empty_init() if h is None else h

    from parallel import utility as pu
    if chunksize is None:
        chunksize = pu.calculate_chunksize(cores)

    global shared_data
    # get reference to output data
    # if different shape it will get the reference to the new array
    shared_data = data[:]

    global second_shared_data
    second_shared_data = second_data[:]

    from multiprocessing import Pool
    pool = Pool(cores)
    img_num = data.shape[0]

    if show_timer:
        h.prog_init(img_num, name + " " + str(cores) +
                    "c " + str(chunksize) + "chs")

    indices_list = pu.generate_indices(img_num)
    for _ in enumerate(pool.imap(partial_func, indices_list, chunksize=chunksize)):
        h.prog_update()

    pool.close()
    pool.join()
    h.prog_close()

    return shared_data, second_shared_data
