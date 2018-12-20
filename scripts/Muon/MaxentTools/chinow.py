
from __future__ import (absolute_import, division, print_function)
import numpy as np
from Muon.MaxentTools.chosol import  CHOSOL

# translation of chinow.for
"""

      END
"""
# JSL: ASSUME all of arrays SPACE_c1 etc are used, i.e. m=3 and size=3


def CHINOW(ax, SPACE_c1, SPACE_c2, SPACE_s1, SPACE_s2, mylog):
    bx = 1. - ax
    a = bx * SPACE_c2 - ax * SPACE_s2
    b = -(bx * SPACE_c1 - ax * SPACE_s1)
    SPACE_beta = CHOSOL(a, b, mylog)
    z = np.dot(SPACE_c2, SPACE_beta)
    w = np.sum(SPACE_beta * (SPACE_c1 + 0.5 * z))
    return 1. + w, SPACE_beta
