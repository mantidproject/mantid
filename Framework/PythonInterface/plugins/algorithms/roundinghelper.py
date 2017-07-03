from __future__ import (absolute_import, division, print_function)
from mantid.kernel import Direction, StringListValidator
import numpy

'''
This file contains functions which deal with rounding in algorithms
such as BinWidthAtX and MedianBinWidth.

'''

# Name of the rounding mode property
PROP_NAME_ROUNDING_MODE = 'Rounding'

# Available rounding modes.
ROUNDING_NONE = 'None'
ROUNDING_TEN_TO_INT = '10^n'


def declare_rounding_property(o):
    '''
    Declares the properties needed for rounding.
    '''
    rounding = StringListValidator()
    rounding.addAllowedValue(ROUNDING_NONE)
    rounding.addAllowedValue(ROUNDING_TEN_TO_INT)
    o.declareProperty(name=PROP_NAME_ROUNDING_MODE,
                      defaultValue=ROUNDING_NONE, validator=rounding,
                      direction=Direction.Input, doc='Bin width rounding')


def round(x, mode):
    '''
    Rounds x depending on the rounding mode selected.
    '''
    if mode == ROUNDING_TEN_TO_INT:
        rounded = 10.0**numpy.floor(numpy.log10(numpy.abs(x)))
        return numpy.copysign(rounded, x)
    return x
