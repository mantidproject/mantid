from __future__ import (print_function, absolute_import, division)
from multiprocessing import freeze_support

import numpy as np


def median_filter(data, size=3, mode='dd'):
    # this will perform the median filter function on a part of the whole data
    # so that the whole data can be processed in parallel
    return data + 3.


def median_filter1(data, size=3, mode='dd'):
    # this will perform the median filter function on a part of the whole data
    # so that the whole data can be processed in parallel
    return data[:, :, :] + 3.


def calculate_indices(c, n):
    """
    This function will create a list with length == number of cores,
    splitting the indices equally per each core.

    :param c: Cores
    :param n: The number of images

    >>> [calculate_indices(n, 30) for n in range(1, 5)]
    [[0, 30], [0, 15, 30], [0, 10, 20, 30], [0, 7, 14, 21, 30]]
    >>> [calculate_indices(n, 30) for n in range(5, 8)]
    [[0, 6, 12, 18, 24, 30], [0, 5, 10, 15, 20, 25, 30], [0, 4, 8, 12, 16, 20, 24, 30]]
    """
    w = int(n / c)  # divide work per core
    if w == 0:
        return [0, n]

    e = n - (w * c)  # leftover indices when we can't divide properly

    l = []
    for i in range(0, (c * w) + 1, w):
        l.append(i)

    l[-1] += e
    return l


def run(cores, data, f, *args, **kwargs):
    s = calculate_indices(cores, data.shape[0])
    from multiprocessing import Pool

    pool = Pool(cores)
    import os
    for i in range(len(s) - 1):
        st = s[i]
        ed = s[i + 1]

        res = pool.apply_async(median_filter1, (data[st:ed, :, :],))
        data[st:ed, :, :] = res.get()

    pool.close()
    pool.join()


def run_parallel(cores, data, f, *args, **kwargs):
    from multiprocessing import Pool
    pool = Pool(cores)
    res = pool.map(median_filter, data)
    # pool.apply(median_filter, (data, 3, 'med',))
    pool.close()
    pool.join()
    for i, d in enumerate(res):
        data[i] = d


def run_parallel_apply(cores, data, f, *args, **kwargs):
    from multiprocessing import Pool
    pool = Pool(cores)
    for i in range(data.shape[0]):
        res = pool.apply_async(median_filter, (data[i],))
        data[i] = res.get()
    # pool.apply(median_filter, (data, 3, 'med',))
    pool.close()
    pool.join()


def main():
    import argparse
    args = argparse.ArgumentParser()
    args.add_argument("-c", "--cores", type=int, required=True)
    args.add_argument("-n", "--num-images", type=int, default=30)
    args = args.parse_args()

    import timeit
    print(timeit.timeit(stmt='run('+str(args.cores)+', data, median_filter, 3, "median_mode")',
                        setup='from __main__ import data, run, median_filter', number=args.num_images))

    print(timeit.timeit(stmt='run_parallel('+str(args.cores)+', data, median_filter, 3, "median_mode")',
                        setup='from __main__ import data, run_parallel, median_filter',
                        number=args.num_images))

    print(timeit.timeit(stmt='run_parallel_apply('+str(args.cores)+', data, median_filter, 3, "median_mode")',
                        setup='from __main__ import data, run_parallel_apply, median_filter',
                        number=args.num_images))

# for i in range(data.shape[0]):
# print("i", data[i])
# print(data)
data = np.full((30, 1000, 1000), 1.)

if __name__ == '__main__':
    freeze_support()
    main()

