from __future__ import (absolute_import, division, print_function)


def deepcopy(data):
    """
    Creates an identical shared array as the input one.
    The input array WILL NOT BE CHANGED!
    Same shape, same data, everything is deep copied.

    :param data: the shared data array
    :return: the copied array
    """
    import numpy as np

    # sanity check
    assert isinstance(data, np.ndarray)

    from parallel import utility as pu
    new_array = pu.create_shared_array(data.shape)
    import copy
    # create a not shared deep copy
    not_shared_copy = copy.deepcopy(data)

    # now we have to move the data to a shared array
    # really inefficient, but I couldn't think of a better way
    # the expected data should be small so it's an okay initial implementation
    new_array[:] = not_shared_copy[:]

    return new_array