# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import heapq


class ColorQueue(object):
    def __init__(self):
        self._queue = []
        self._max_index = 0

    def __call__(self):
        if len(self._queue) != 0:
            return 'C' + str(heapq.heappop(self._queue))
        else:
            return_value = 'C' + str(self._max_index)
            self._max_index = self._max_index + 1
            return return_value

    def __add__(self, color):
        # Could use rexp to do these checks but feels
        # like overkill in this case.
        if color[0] != 'C':
            return self
        try:
            colour_index = int(color[1:])
        except ValueError:
            return self
        heapq.heappush(self._queue, colour_index)
        return self

    def reset(self):
        self._max_index = 0
        self._queue = []
