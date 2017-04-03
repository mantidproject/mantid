from __future__ import (absolute_import, division, print_function)
import helper as h
from core.parallel import utility as pu
from multiprocessing import Pool


def create_partial(func, **kwargs):
    """
    Create a partial using functools.partial, to forward the kwargs to the parallel execution of imap.

    This version does not have a forwarding function, because one is not necessary.
    The data is copied regardless of it being forwarded or not.

    :param func: Function that will be executed
    :param kwargs: kwargs to forward to the function func that will be executed
    :return: a constructed partial object
    """
    from functools import partial
    return partial(func, **kwargs)


def execute(data=None,
            partial_func=None,
            cores=None,
            chunksize=None,
            name="Progress",
            output_data=None,
            show_timer=True):
    """
    Executes a function in parallel, but does not share the memory between processes.
    Every process will copy-on-read/write the data to its own virtual memory region, perform the calculation
    and return the result to the main process, where it will be moved (not copied) to the original container.

    - imap_unordered gives the images back in random order!
    - map and map_async cannot replace the data in place and end up
    doubling the memory. They do not improve speed performance either
    - imap seems to be the best choice

    This also means that this class potentially uses MUCH more memory, and performs a lot slower.

    If you get a similar error:
        output_data[i] = res_data[:]
    TypeError: 'NoneType' object has no attribute '__getitem__'

    It means that the forwarded function is not returning anything and it should!

    :param data: the data array that will be processed in parallel
    :param partial_func: a function constructed using partial to pass the correct arguments
    :param cores: number of cores that the processing will use
    :param chunksize: chunk of work per process(worker)
    :param name: the string that will be appended in front of the progress bar
    :param output_data: the output array in which the results will be stored
    :param show_timer: Whether to show the loading bar or be completely silent

    :return: The processed output data
    """
    if not cores:
        cores = pu.get_cores()

    if not chunksize:
        chunksize = pu.calculate_chunksize(cores)

    # handle the case of having a different output that input i.e. rebin,
    # crop, etc
    if output_data is None:
        # get data reference to original with [:]
        output_data = data

    pool = Pool(cores)
    img_num = output_data.shape[0]
    if show_timer:
        h.prog_init(img_num,
                    name + " " + str(cores) + "c " + str(chunksize) + "chs")

    # passing the data triggers a copy-on-write in the child process, even if
    # it only reads the data
    for i, res_data in enumerate(
            pool.imap(partial_func, data, chunksize=chunksize)):
        output_data[i] = res_data[:]
        h.prog_update()

    pool.close()
    pool.join()
    h.prog_close()

    return output_data
