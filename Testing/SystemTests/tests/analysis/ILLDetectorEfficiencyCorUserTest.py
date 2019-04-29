# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import numpy
import numpy.testing
import systemtesting
from mantid.simpleapi import (ConvertUnits, DetectorEfficiencyCorUser, DirectILLCollectData)


class IN4(systemtesting.MantidSystemTest):
    def runTest(self):
        ws = DirectILLCollectData('ILL/IN4/084446.nxs', ElasticChannel='Default Elastic Channel',
                                  FlatBkg='Flat Bkg OFF', Normalisation='Normalisation OFF')
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i).fill(1)
        dE = ConvertUnits(ws, 'DeltaE', 'Direct')
        corr = DetectorEfficiencyCorUser(dE)
        Ei = corr.run().get('Ei').value

        def rosace_corr(x):
            return 1.0 - numpy.exp(-6.1343 / numpy.sqrt(x))

        def wide_angle_corr(x):
            return 0.951 * numpy.exp(-0.0887 / numpy.sqrt(x)) * (1 - numpy.exp(-5.597 / numpy.sqrt(x)))

        def eff_factor(x, corr_func):
            return corr_func(Ei) / corr_func(x)

        # Wide-angle detectors are at ws indices 0-299
        for i in range(0, 300):
            x = (corr.readX(i)[:-1] + corr.readX(i)[1:]) / 2.
            e = Ei - x
            numpy.testing.assert_almost_equal(corr.readY(i), eff_factor(e, wide_angle_corr))
        # Rosace detectors are at ws indices 300-395
        for i in range(300, 396):
            x = (corr.readX(i)[:-1] + corr.readX(i)[1:]) / 2.
            e = Ei - x
            numpy.testing.assert_almost_equal(corr.readY(i), eff_factor(e, rosace_corr))


class IN5(systemtesting.MantidSystemTest):
    def runTest(self):
        ws = DirectILLCollectData('ILL/IN5/104007.nxs', ElasticChannel='Default Elastic Channel',
                                  FlatBkg='Flat Bkg OFF', Normalisation='Normalisation OFF')
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i).fill(1)
        dE = ConvertUnits(ws, 'DeltaE', 'Direct')
        corr = DetectorEfficiencyCorUser(dE)
        Ei = corr.run().get('Ei').value

        def tube_corr(x):
            return 1. - numpy.exp(-5.6 / numpy.sqrt(x))

        corr_at_Ei = tube_corr(Ei)
        for i in range(corr.getNumberHistograms()):
            x = (corr.readX(i)[:-1] + corr.readX(i)[1:]) / 2.
            e = Ei - x
            numpy.testing.assert_almost_equal(corr.readY(i), corr_at_Ei / tube_corr(e))


class IN6(systemtesting.MantidSystemTest):
    def runTest(self):
        ws = DirectILLCollectData('ILL/IN6/164192.nxs', ElasticChannel='Default Elastic Channel',
                                  FlatBkg='Flat Bkg OFF', Normalisation='Normalisation OFF')
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i).fill(1)
        dE = ConvertUnits(ws, 'DeltaE', 'Direct')
        corr = DetectorEfficiencyCorUser(dE)
        Ei = corr.run().get('Ei').value

        def det_corr(x):
            high = x > 5.113
            low = x <= 5.113
            c = numpy.empty_like(x)
            c[high] = 0.94 * (1. - numpy.exp(-3.284/numpy.sqrt(x[high])))
            c[low] = numpy.exp(-0.0565 / numpy.sqrt(x[low])) * (1.- numpy.exp(-3.284 / numpy.sqrt(x[low])))
            return c

        corr_at_Ei = det_corr(numpy.array([Ei]))[0]
        for i in range(corr.getNumberHistograms()):
            x = (corr.readX(i)[:-1] + corr.readX(i)[1:]) / 2.
            e = Ei - x
            numpy.testing.assert_almost_equal(corr.readY(i), corr_at_Ei / det_corr(e))
