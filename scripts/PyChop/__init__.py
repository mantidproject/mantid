# pylint: disable=line-too-long, invalid-name

"""
Module containing classes to calculate the flux and resolution of direct geometry neutron time-of-flight spectrometers.
At present, the subclasses understand the ISIS instruments 'LET', 'MAPS', 'MARI' and 'MERLIN'.

Usage:

from PyChop import PyChop2

merlin = PyChop2('merlin','s',250)
merlin.setEi(25)
resolution = merlin.getResolution()

resolution, flux = PyChop2.calculate(inst='maps', chtyp='a', freq=500, ei=600, etrans=range(0,550,50))

PyChop2.showGUI()
"""

from __future__ import (absolute_import, division, print_function)
import warnings
from .PyChop2 import PyChop2
# If the system doesn't have matplotlib, don't import the GUI.
try:
    from .PyChopGui import show as showGUI
except ImportError:
    def showGUI():
        warnings.warn("PyChop GUI disabled: Cannot import Matplotlib.", RuntimeWarning)
        return None
