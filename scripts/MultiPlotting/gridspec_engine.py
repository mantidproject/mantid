from __future__ import (absolute_import, division, print_function)

from matplotlib.gridspec import GridSpec
import math


def defaultGridspecGrid(number):
    if number == 3:
        return [3, 1]

    root = math.sqrt(number)
    if root - math.floor(root) == 0.0:
        return [int(root), int(root)]

    col = int(math.ceil(root))
    row = col - 1
    if number <= row * col:
        return [row, col]
    else:
        return [col, col]


class gridspecEngine(object):

    def __init__(self, grid=defaultGridspecGrid, max_plot=None):
        self._max_plot = max_plot
        self.grid = grid

    def getGridSpec(self, number):
        if number <= 0:
            return
        if self._max_plot is not None and number > self._max_plot:
                print(
                    "Number of plot has exceeded the maximum number of " + str(self._max_plot))
                return
        current_grid = self.grid(number)
        return GridSpec(current_grid[0], current_grid[1])
