from __future__ import (absolute_import, division, print_function)
import sys
import os


class CustomTimer(object):
    """
    Custom fallback implementation for timer that provides only a basic loading bar.
    This timer bar will be used if tqdm is not present of the system.
    """

    def __init__(self, total, prefix=''):
        self._total = total
        self._iter = 0
        self._prefix = prefix + ": "
        self._bar_len = 60

    def update(self, iterations):
        self._iter += iterations
        # how much is filled
        rows, columns = os.popen('stty size', 'r').read().split()

        self._bar_len = int(int(columns) / 2)

        filled_len = int(
            round(self._bar_len * self._iter / float(self._total)))
        # what percentage is done
        percent = round(100.0 * self._iter / float(self._total), 1)
        bar = self._prefix + '[' + '=' * filled_len + \
            '-' * (self._bar_len - filled_len) + \
            "]" + str(percent) + ' / ' + str(self._total)

        print(bar, end='\r')
        sys.stdout.flush()

        if self._iter == self._total:
            print()

    def close(self):
        pass
