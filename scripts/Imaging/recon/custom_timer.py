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
        self._bar_len = 40

    def update(self, iterations):
        self._iter += iterations

        # this can cause random crashes on SCARF
        # rows, columns = os.popen('stty size', 'r').read().split()
        # give a default length

        filled_len = int(
            round(self._bar_len * self._iter / float(self._total)))

        bar = self._prefix + '[' + '=' * filled_len + \
            '-' * (self._bar_len - filled_len) + \
            "]" + str(self._iter) + ' / ' + str(self._total)

        print(bar, end='\r')
        sys.stdout.flush()

        if self._iter == self._total:
            print()

    def close(self):
        pass
