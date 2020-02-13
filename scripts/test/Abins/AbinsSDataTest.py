# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import six
import unittest
import numpy as np
import logging
from AbinsModules import SData, AbinsParameters


class AbinsSDataTest(unittest.TestCase):
    def setUp(self):
        self.default_threshold_value = AbinsParameters.sampling['s_absolute_threshold']
        self.default_min_wavenumber = AbinsParameters.sampling['min_wavenumber']
        self.default_max_wavenumber = AbinsParameters.sampling['max_wavenumber']
        self.logger = logging.getLogger('abins-sdata-test')

    def tearDown(self):
        AbinsParameters.sampling['s_absolute_threshold'] = self.default_threshold_value
        AbinsParameters.sampling['min_wavenumber'] = self.default_min_wavenumber
        AbinsParameters.sampling['max_wavenumber'] = self.default_max_wavenumber

    def test_s_data(self):
        AbinsParameters.sampling['min_wavenumber'] = 100
        AbinsParameters.sampling['max_wavenumber'] = 150

        s_data = SData(temperature=10, sample_form='Powder')
        s_data.set_bin_width(10)
        s_data.set({'frequencies': np.linspace(105, 145, 5),
                    'atom_1': {'s': {'order_1': np.array([0., 0.001, 1., 1., 0., 0.,])}}})

        if six.PY3: # assertLogs is available from Python 3.4 and up
            with self.assertRaises(AssertionError):
                with self.assertLogs(logger=self.logger, level='WARNING'):
                    s_data.check_thresholds(logger=self.logger)

            AbinsParameters.sampling['s_absolute_threshold'] = 0.5
            with self.assertLogs(logger=self.logger, level='WARNING'):
                s_data.check_thresholds(logger=self.logger)
