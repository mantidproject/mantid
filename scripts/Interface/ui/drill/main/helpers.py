# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

TECHNIQUE_MAP = {'D11': 'sans', 'D16': 'sans', 'D22': 'sans', 'D33': 'sans',
                 'D17': 'refl', 'FIGARO': 'refl'}


def getTechnique(instrument):

    if instrument in TECHNIQUE_MAP.keys():
        return TECHNIQUE_MAP[instrument]
    else:
        raise RuntimeError('Instrument {0} is not yet supported.'.format(instrument))
