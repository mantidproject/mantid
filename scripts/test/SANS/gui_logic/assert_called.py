# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


def assert_called(func, n=1):
    # Calls to this method with n = 1 can
    # be replaced with a call to mock.assert_called
    # when python 2 support is dropped.
    assert (func.call_count >= n)
