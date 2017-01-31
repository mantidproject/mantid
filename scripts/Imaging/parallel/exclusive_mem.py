from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper
import numpy as np

"""
The difference between recon.parallel and recon.shared_parallel is that the latter uses a shared memory array between
the processes, to avoid copy-on-read/write the data to each process' virtual memory space.

This also means that this class potentially uses more memory.

Exclusive memory needs to be used when the input array has a different shape from the input array.
However they can only have different shapes in the X and Y dimensions. The number of images (Z) must remain the same.
"""


def create_partial(func, **kwargs):
    """
    Create a partial using functools.partial, to forward the kwargs to the parallel execution of imap.

    This version does not have a forwarding function, because one is not necessary.

    :param func: Function that will be executed
    :param kwargs: kwargs to forward to the function func that will be executed
    :return: a constructed partial object
    """
    from functools import partial
    return partial(func, **kwargs)


def execute(data=None, partial_func=None, cores=1, chunksize=None, name="Progress", h=None, output_data=None,
            show_timer=True):
    """
    Executes a function in parallel, but does not share the memory between processes.
    Every process will copy-on-read/write the data to its own virtual memory region, perform the calculation
    and return the result to the main process, where it will be moved (not copied) to the original container.


    - imap_unordered gives the images back in random order!
    - map and map_async cannot replace the data in place and end up
    doubling the memory. They do not improve speed performance either
    - imap seems to be the best choice


    :param data: the data array that will be processed in parallel
    :param partial_func: a function constructed using partial to pass the correct arguments
    :param cores: number of cores that the processing will use
    :param chunksize: chunk of work per process(worker)
    :param name: the string that will be appended in front of the progress bar
    :param h: the helper class
    :param output_data: the output array in which the results will be stored
    :return:
    """
    h = Helper.empty_init() if h is None else h

    from parallel import utility as pu
    if chunksize is None:
        chunksize = pu.calculate_chunksize(cores)

    # handle the edge case of having a different output that input i.e. rebin, crop, etc
    if output_data is None:
        # get data reference to original with [:]
        output_data = data[:]

    from multiprocessing import Pool

    pool = Pool(cores)
    img_num = output_data.shape[0]
    if show_timer:
        h.prog_init(img_num, name + " " + str(cores) +
                    "c " + str(chunksize) + "chs")

    # passing the data triggers a copy-on-write in the child process, even if it only reads the data
    for i, res_data in enumerate(pool.imap(partial_func, data, chunksize=chunksize)):
        output_data[i] = res_data[:]
        h.prog_update()

    pool.close()
    pool.join()
    h.prog_close()

    return output_data
