# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

"""
         ngroups, npts are array sizes
         hists(ngroups): integer flag to say if used or not? R
         P (integer) - not used?
         Datum (npts,ngroups) = counts, R (and occasionally W)
         Sigma (npts,ngroups) = error values R
         corr(npts,ngroups) - not used?
"""


def BACK(hists, datum, sigma, DETECT_e, filePHASE, mylog):
    (npts, ngroups) = datum.shape
    DETECT_d = np.zeros([ngroups])
    for j in range(ngroups):
        scale = 0
        if hists[j] > 0:
            Ax = np.sum(DETECT_e * datum[:, j] / sigma[:, j] ** 2)
            Bx = np.sum(DETECT_e**2 / sigma[:, j] ** 2)
            scale = Ax / Bx
        DETECT_d[j] = scale
        datum[:, j] = np.where(sigma[:, j] > 1.0e6, datum[:, j], datum[:, j] - scale * DETECT_e)
    ggro = 0.0
    sum = 0.0
    for j in range(ngroups):
        if hists[j] > 0:
            ggro = ggro + 1.0
            sum = sum + DETECT_d[j]
    sum = sum / ggro
    amp = np.where(hists > 0, DETECT_d / sum, 0.0)
    # read PHASE.DAT, was in degrees, one number per line. Present filePHASE
    # is in radians
    FASE_phase = filePHASE
    DETECT_a = amp * np.cos(FASE_phase)
    DETECT_b = amp * np.sin(FASE_phase)
    # convert to self.log().information() if self is passed in
    mylog.debug("phases as loaded" + str(filePHASE))
    mylog.debug("exponentials as fitted" + str(DETECT_d))
    return datum, DETECT_a, DETECT_b, DETECT_d, FASE_phase
