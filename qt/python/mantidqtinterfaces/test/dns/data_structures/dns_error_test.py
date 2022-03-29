# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.dns.data_structures.dns_error import DNSError


class DNSErrorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.error = DNSError()

    def test___init__(self):
        self.assertIsInstance(self.error, BaseException)


if __name__ == '__main__':
    unittest.main()
