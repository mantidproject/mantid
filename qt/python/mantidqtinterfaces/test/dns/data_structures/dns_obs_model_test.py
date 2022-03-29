# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns.data_structures.dns_obs_model import DNSObsModel


class DNSObsModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.parent = 'test'
        cls.model = DNSObsModel(parent=cls.parent)

    def test___init__(self):
        self.assertIsInstance(self.model, object)
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertEqual(self.model.parent, 'test')


if __name__ == '__main__':
    unittest.main()
