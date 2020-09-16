# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# See https://docs.python.org/3/library/heapq.html for details on heapq
import heapq


class ColorQueue(object):
    def __init__(self, color_list):
        """Create a ColorQueue based on an input color_list
        The entry at index 0 has highest priority
        color_list could be an input list of color codes
        e.g ['C0', 'C1', 'C2']
        or a list of hex colors
        e.g [#000000,#FF0000....]
        or rgb values
        e.g [(0, 0, 0), (255, 0, 0)..]
        All the above are supported by matplotlib
        """
        # Create a cache of the color list, where each entry is a {color: priority} pair
        self._color_cache = {color_list[i]: i for i in range(len(color_list))}
        self._initialise_queue()

    def reset(self):
        """Reset the queue based on the stored color_cache"""
        self._initialise_queue()

    def __call__(self):
        """Pop the highest priority color off the queue"""
        if len(self._queue) == 0:
            self._initialise_queue()
        return heapq.heappop(self._queue)[1]

    def __add__(self, color):
        """Add a color into the queue, with its priority found from the stored cache
        If its a duplicate color, it will be ignored"""
        if not any(self._queue[i][1] == color for i in range(len(self._queue))):
            heapq.heappush(self._queue, (self._get_color_priority(color), color))
        return self

    def _initialise_queue(self):
        """Initialize the heap queue with the color_cache"""
        self._queue = []
        for color, priority in self._color_cache.items():
            heapq.heappush(self._queue, (priority, color))

    def _get_color_priority(self, color):
        """Return the priority of a color, if it is not present in the cache
        set the priority so that it is in the back of the queue"""
        return self._color_cache.get(color, max(len(self._queue), len(self._color_cache)) + 1)
