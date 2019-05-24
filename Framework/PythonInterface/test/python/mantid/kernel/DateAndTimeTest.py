# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import DateAndTime
import numpy
from numpy import timedelta64, datetime64
from distutils.version import LooseVersion


class DateAndTimeTest(unittest.TestCase):

    iso_str = "2008-12-18T17:58:38"
    # We had to add a space to the end of the string representation to get around an IPython bug (#8351)
    iso_str_plus_space = iso_str + " "

    def test_construction_with_ISO_string_produces_expected_object(self):
        dt = DateAndTime(self.iso_str)
        self.assertEquals(self.iso_str_plus_space, str(dt))
        self.assertEquals(dt.totalNanoseconds(), 598471118000000000)

    def test_construction_with_total_nano_seconds(self):
        dt = DateAndTime(598471118000000000)
        self.assertEquals(self.iso_str_plus_space, str(dt))

    def test_convert_to_np(self):
        dt = DateAndTime(598471118000000000)
        dt_np = timedelta64(dt.totalNanoseconds(), 'ns') + datetime64('1990-01-01T00:00')

        # convert both into ISO8601 strings up to the seconds
        dt = str(dt)[:19]
        dt_np = str(dt_np)[:19]
        self.assertEquals(dt, dt_np)

    def test_convert_from_np(self):
        if LooseVersion(numpy.__version__) < LooseVersion('1.9'):
            dt_np = datetime64('2000-01-01T00:00Z')
        else: # newer numpy only uses UTC and warns on specifying timezones
            dt_np = datetime64('2000-01-01T00:00')
        dt = DateAndTime(dt_np)

        # convert both into ISO8601 strings up to the minutes b/c time was only specified that much
        dt = str(dt)
        dt_np = dt_np.item().strftime('%Y-%m-%dT%M:%S')
        length = min(len(dt), len(dt_np))
        dt = dt[:length]
        dt_np = dt_np[:length]
        self.assertEquals(dt, dt_np)


if __name__ == "__main__":
    unittest.main()
