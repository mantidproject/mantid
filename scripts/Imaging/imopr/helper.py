from __future__ import (absolute_import, division, print_function)

# TODO should change the name of this file so its not confused with
# the other helper.py that's not in imopr


def print_start(action_name):
    print("*********************************************\n*\n*     " +
          action_name + "\n*\n*********************************************")


def handle_indices(indices):
    i1 = indices[0]
    try:
        i2 = indices[1]
    except IndexError:
        i2 = i1 + 1

    return i1, i2
