from __future__ import (absolute_import, division, print_function)
import numpy as np
import numpy.testing as npt
from core.parallel import utility as pu

backup_mp_avail = None
g_shape = (10, 8, 10)


def gimme_shape():
    return g_shape


def gen_img_numpy_rand():
    return np.random.rand(*g_shape)


def gen_img_shared_array_and_copy(shape=g_shape):
    arr = gen_img_shared_array(shape)
    copy = shared_deepcopy(arr)
    return arr, copy


def gen_img_shared_array(shape=g_shape):
    d = pu.create_shared_array(shape)
    n = np.random.rand(shape[0], shape[1], shape[2])
    d[:] = n[:]

    return d


def gen_img_shared_array_with_val(val=1., shape=g_shape):
    d = pu.create_shared_array(shape)
    n = np.full(shape, val)
    d[:] = n[:]

    return d


def assert_equals(thing1, thing2):
    npt.assert_equal(thing1, thing2)


def assert_not_equals(thing1, thing2):
    npt.assert_raises(AssertionError, npt.assert_equal, thing1, thing2)


def deepcopy(source):
    from copy import deepcopy
    return deepcopy(source)


def shared_deepcopy(source):
    d = pu.create_shared_array(source.shape)
    from copy import deepcopy
    d[:] = deepcopy(source)[:]
    return d


def debug(switch=True):
    if switch:
        import pydevd
        pydevd.settrace(
            'localhost', port=59003, stdoutToServer=True, stderrToServer=True)


def vsdebug():
    import ptvsd
    ptvsd.enable_attach("my_secret", address=('0.0.0.0', 59003))
    print("Waiting for remote debugger at localhost:59003")
    #Enable the below line of code only if you want the application to wait untill the debugger has attached to it
    ptvsd.wait_for_attach()


def switch_mp_off():
    """
    This function does very bad things that should never be replicated.
    But it's a unit test so it's fine.
    """
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
    # restore the original backed up function from switch_mp_off
    pu.multiprocessing_available = backup_mp_avail
