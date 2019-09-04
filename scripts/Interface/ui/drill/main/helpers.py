# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

TECHNIQUE_MAP = {'D11' : 'SANS', 'D22' : 'SANS', 'D33' : 'SANS', 'D17' : 'Reflectometry', 'FIGARO' : 'Reflectometry'}

def getTechnique(instrument):

    if instrument in TECHNIQUE_MAP.keys():
        return TECHNIQUE_MAP[instrument]
    else:
        raise RuntimeError('Unsupported instrument {0}'.format(instrument))
