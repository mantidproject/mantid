
from __future__ import (absolute_import, division, print_function)
import math

from Muon.MaxentTools.chinow import CHINOW
from Muon.MaxentTools.dist import DIST


# variable p is used? Size of sigma

def MOVE(
     sigma,
     SPACE_chisq,
     SPACE_chizer,
     SPACE_xsum,
     SPACE_c1,
     SPACE_c2,
     SPACE_s1,
     SPACE_s2,
     SPACE_blank,
     FAC_factor,
     FAC_facfake,
     mylog):
    (cmin, SPACE_beta) = CHINOW(0., SPACE_c1,
                                SPACE_c2, SPACE_s1, SPACE_s2, mylog)
    if(cmin * SPACE_chisq > SPACE_chizer):
        ctarg = 0.5 * (1. + cmin)
    else:
        ctarg = SPACE_chizer / SPACE_chisq
    a1 = 0.
    a2 = 1.
    jtest = 0.
    f1 = cmin - ctarg
    (f2, SPACE_beta) = CHINOW(1., SPACE_c1,
                              SPACE_c2, SPACE_s1, SPACE_s2, mylog)
    f2 = f2 - ctarg
    while(True):
        anew = 0.5 * (a1 + a2)
        (fx, SPACE_beta) = CHINOW(anew, SPACE_c1,
                                  SPACE_c2, SPACE_s1, SPACE_s2, mylog)
        fx = fx - ctarg
        if(f1 * fx > 0):
            a1 = anew
            f1 = fx
        if(f2 * fx > 0):
            a2 = anew
            f2 = fx
        if(abs(fx) >= 1.E-3):
            jtest = jtest + 1
            if(jtest > 10000):
                mylog.notice(' stuck in MOVE : chi**2 not tight enough')
                sigma = sigma * 0.99
                FAC_factor = FAC_factor * 0.99
                # FAC_facdef=FAC_factor
                FAC_facfake = FAC_facfake * 0.99
                mylog.notice(
                    ' tightening looseness factor by 1 % to: {0}'.format(FAC_factor))
                break
        else:
            break
    w = DIST(SPACE_beta, SPACE_s2)
    if(w > 0.1 * SPACE_xsum / SPACE_blank):
        SPACE_beta = SPACE_beta * \
            math.sqrt(0.1 * SPACE_xsum / (SPACE_blank * w))
    SPACE_chtarg = ctarg * SPACE_chisq
    return sigma, SPACE_chtarg, SPACE_beta, FAC_factor, FAC_facfake
