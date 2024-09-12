# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name

"""
Module containing classes to calculate the flux and resolution of direct geometry neutron time-of-flight spectrometers.
At present, the subclasses understand the ISIS instruments 'LET', 'MAPS', 'MARI' and 'MERLIN'.

Usage:

from pychop.Instruments import Instrument

merlin = Instrument('merlin','s',250)
merlin.setEi(25)
resolution = merlin.getResolution()

resolution, flux = Instrument.calculate(inst='maps', chtyp='a', freq=500, ei=600, etrans=range(0,550,50))
"""

from pychop import PyChop2  # noqa
