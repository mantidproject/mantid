from __future__ import (absolute_import, division, print_function)
import ctypes
import multiprocessing
import numpy as np
import numpy.testing as npt
import unittest

from recon.configs.recon_config import ReconstructionConfig
from recon.helper import Helper


class ParallelTest(unittest.TestCase):

    def test_shared_performance(self, runs=10, cores='8', chunksize='8'):
        import timeit
        print("shared_perf:", timeit.timeit(
            stmt='sp.execute(data,f,h=h,cores=' + cores +
            ',chunksize=' + chunksize + ',show_timer=False)',
            setup='from recon.memory_debugger import inplace_in_func, '
                  '_create_testing_array, _create_test_helper;'
                  'from recon.parallel import shared_parallel as sp;'
                  'data = _create_testing_array();'
                  'h = _create_test_helper();'
                  'f = sp.create_partial(inplace_in_func, fwd_function=sp.fwd_func, size=42);',
            number=runs), end='')

    def test_not_shared_performance(self, runs=10, cores='8', chunksize='8'):
        import timeit
        print("not shared_perf:", timeit.timeit(
            stmt='p.execute(data,f,h=h,cores=' + cores +
            ',chunksize=' + chunksize + ',show_timer=False)',
            setup='from recon.memory_debugger import return_from_func_but_data_changed_inside, '
                  '_create_testing_array, _create_test_helper; '
                  'from recon.parallel import parallel as p;'
                  'import numpy as np;'
                  'data = np.full((5,10,10), 1.);'
                  'h = _create_test_helper();'
                  'f = p.create_partial(return_from_func_but_data_changed_inside, size=42)', number=runs), end='')

    def test_fwd_func_processing_inplace(self):
        """
        Test if forwarding a function that expects a return from the function,
        but doesn't get one, turns all the data into nans

        Expected: The data should be all nans
        """
        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        from copy import deepcopy
        control = deepcopy(data)
        control[:] = np.nan

        h = _create_test_helper()

        f = sp.create_partial(
            inplace_in_func, fwd_function=sp.fwd_func, size=42)

        data = sp.execute(data, f, h=h, show_timer=False)
        npt.assert_equal(data, control)

    def test_fwd_func_processing_not_inplace(self):
        """
        Test if using fwd_func with a function that doesn't change the data in place returns the proper result

        Expected: The data should be correctly changed
        """
        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        from copy import deepcopy
        control = deepcopy(data)

        h = _create_test_helper()

        # uses a a partial function that DOES NOT change the data inside
        f = sp.create_partial(
            return_from_func_no_data_change_inside, fwd_function=sp.fwd_func, size=42)

        data = sp.execute(data, f, h=h, show_timer=False)

        npt.assert_equal(data, control)

    def test_fwd_func_processing_not_inplace2(self):
        """
        Test if using fwd_func with a function that doesn't change the data in place returns the proper result

        Expected: The data should be correctly changed
        """
        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        from copy import deepcopy
        control = deepcopy(data)

        h = _create_test_helper()

        # uses a different partial function, that DOES change the data inside
        f = sp.create_partial(
            return_from_func_but_data_changed_inside, fwd_function=sp.fwd_func, size=42)

        data = sp.execute(data, f, h=h, show_timer=False)

        npt.assert_raises(AssertionError, npt.assert_equal, data, control)

    def test_inplace_fwd_func_processing_inplace(self):
        """
        Test if using inplace_fwd_func with a function that changes the data inplace gives proper result

        Expected: The data should be correctly changed
        """
        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        from copy import deepcopy
        control = deepcopy(data)

        h = _create_test_helper()

        f = sp.create_partial(
            inplace_in_func, fwd_function=sp.inplace_fwd_func, size=42)

        data = sp.execute(data, f, show_timer=False, h=h)
        npt.assert_raises(AssertionError, npt.assert_equal, data, control)

    def test_inplace_fwd_func_processing_not_inplace(self):
        """
        Test if inplace_fwd_func with a function that does return data

        Expected: The data should not be changed
        """

        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        h = _create_test_helper()

        from copy import deepcopy
        control = deepcopy(data)

        f = sp.create_partial(
            return_from_func_no_data_change_inside, fwd_function=sp.inplace_fwd_func, size=42)

        data = sp.execute(data, f, show_timer=False, h=h)

        npt.assert_equal(data, control)

    def test_inplace_fwd_func_processing_not_inplace2(self):
        """
        Test if inplace_fwd_func with a function that does return data

        Expected: The data should not be changed
        """

        from recon.parallel import shared_parallel as sp
        data = _create_testing_array()

        h = _create_test_helper()

        from copy import deepcopy
        control = deepcopy(data)

        # uses a different partial function, that changes the data inside,
        # and since it's a shared array it will be changed on the check as well
        f = sp.create_partial(
            return_from_func_but_data_changed_inside, fwd_function=sp.inplace_fwd_func, size=42)

        data = sp.execute(data, f, show_timer=False, h=h)

        npt.assert_raises(AssertionError, npt.assert_equal, data, control)


def _create_test_helper():
    c = ReconstructionConfig.empty_init()
    c.func.verbosity = 3
    return Helper(c)


def _create_testing_array():
    from recon.parallel import shared_parallel as sp

    data = sp.create_shared_array((5, 10, 10))
    for z in range(data.shape[0]):
        for y in range(data.shape[1]):
            for x in range(data.shape[2]):
                data[z, y, x] = y
    return data


def inplace_in_func(func_data, size=3):
    func_data[:] = func_data.sum()


def return_from_func_but_data_changed_inside(func_data, size=3):
    func_data[:] = func_data.sum()
    return func_data


def return_from_func_no_data_change_inside(func_data, size=3):
    return func_data


if __name__ == '__main__':
    unittest.main()
    # test_inplace_fwd_func_processing_not_inplace()
    # big shape tests individual process performance per image
    # test_shape = (50, 512, 512)

    # a lot of runs with small shape will show the overhead for processes
    # runs = 5
    # cores = '8'
    # from cpython source
    # chunksize, extra = divmod(test_shape[0], int(cores) * 4)
    # if extra:
    #     chunksize += 1
    # chunksize = str(chunksize)
    # print("chunksize", chunksize)

    # c = ReconstructionConfig.empty_init()
    # c.func.verbosity = 3
    # h = Helper(c)

    # data = _create_test_shared_array(test_shape)
    # from recon.parallel import shared_parallel as sp
    #
    # f = sp.create_partial(
    #     set_to_inplace, fwd_function=sp.inplace_fwd_func, size=42)
    # for chunk in range(1, 10):
    #     print("chunksize", chunk)
    #
    #     test_shared_performance(runs, cores, str(chunk))

    # reset data
    # data = np.zeros(test_shape)

    from recon.parallel import parallel as p

    # new function with return forwarding
    # f = p.create_partial(set_to_return, size=42)
    # for chunk in range(1, 10):
    #     print("chunksize", chunk)
    #     test_not_shared_performance(runs, cores, str(chunk))
