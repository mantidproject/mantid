# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


def get_layout(num):
    return layout[num - 1]()


def get_layout_four():
    return [[0, 0], [0, 1], [1, 0], [1, 1]]


def get_layout_three():
    return [[0, 0], [1, 0], [2, 0]]


def get_layout_two():
    return [[0, 0], [0, 1]]


def get_layout_one():
    return [[0, 0]]


layout = [get_layout_one, get_layout_two, get_layout_three, get_layout_four]
