from helper import Helper
import numpy as np
import numpy.testing as npt

backup_mp_avail = None


def gen_img_numpy_rand():
    # generate 10 images with dimensions 10x10, all values 1. float32
    return np.random.rand(10, 10, 10)


def gen_img_shared_array_and_copy(shape=(10, 10, 10)):
    arr = gen_img_shared_array(shape)
    copy = deepcopy(arr)
    return arr, copy


def gen_img_shared_array(shape=(10, 10, 10)):
    from parallel import utility as pu
    # generate 10 images with dimensions 10x10, all values 1. float32
    d = pu.create_shared_array(shape)
    n = np.random.rand(shape[0], shape[1], shape[2])

    for i in range(shape[0]):
        d[i] = n[i]

    return d


def assert_equals(thing1, thing2):
    npt.assert_equal(thing1, thing2)


def assert_not_equals(thing1, thing2):
    npt.assert_raises(AssertionError, npt.assert_equal, thing1, thing2)


def deepcopy(source):
    from copy import deepcopy
    return deepcopy(source)


def gimme_helper():
    return Helper.empty_init()


def debug(switch=True):
    if switch:
        import pydevd
        pydevd.settrace('localhost', port=59003,
                        stdoutToServer=True, stderrToServer=True)


def switch_mp_off():
    """
    This function does very bad things that should never be replicated.
    But it's a unit test so it's fine.
    """
    from parallel import utility as pu
    # backup function so we can restore it
    global backup_mp_avail
    backup_mp_avail = pu.multiprocessing_available

    def simple_return_false():
        return False
    # do bad things, swap out the function to one that returns false
    pu.multiprocessing_available = simple_return_false


def switch_mp_on():
    """
    This function does very bad things that should never be replicated.
    But it's a unit test so it's fine.
    """
    from parallel import utility as pu
    # restore the original backed up function from switch_mp_off
    pu.multiprocessing_available = backup_mp_avail
