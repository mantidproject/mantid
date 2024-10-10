#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Plot data in SCDcalib.log file from Mantid SCD Calibration.
A. J. Schultz, V. E. Lynch, August 2015
"""

import pylab
import os
import math
import sys
import numpy as np

sys.path.append("/opt/mantidnightly/bin")
from mantid.simpleapi import Load

# Make a ./plots subdirectory for the plot files.
if not os.path.exists("./plots"):
    os.mkdir("./plots")


OUTPUT_FNAME = "SCDcalib_plot.log"
OUTPUT = open(OUTPUT_FNAME, "w")
OUTPUT.write("RMSD in mm units\n")
OUTPUT.write(" ID  NumPeaks       Row    Column  Combined\n")

XCALC = [0, 255]
YCALC = [0, 255]

# Begin reading and plotting.
WSROW = Load("RowCalcvsTheor.nxs")
WSCOL = Load("ColCalcvsTheor.nxs")

for i in range(WSROW.getNumberHistograms()):
    x = WSROW.readX(i)
    y = WSROW.readY(i)
    y = np.trim_zeros(y, "b")
    ylen = len(y)
    x = x[0:ylen]
    xy = zip(x, y)
    new_xy = ((a, b) for a, b in xy if b >= 0)
    x, y = zip(*new_xy)
    x = np.array(x)
    y = np.array(y)
    chisq_row = np.sum((x - y) ** 2)
    IDnum = WSROW.getSpectrum(i).getSpectrumNo()
    title = "bank" + str(IDnum) + "_Row"

    pylab.plot(x, y, "r+")
    pylab.plot(XCALC, YCALC)

    pylab.xlabel("Calculated Row Number")
    pylab.ylabel("Observed Row Number")
    pylab.grid(True)

    pylab.title(title)

    numPeaks = len(x)
    rmsd_row = math.sqrt((1.0 / numPeaks) * chisq_row)
    rmsd_row_mm = rmsd_row * 150 / 256
    reduced_chisq_row = chisq_row / (numPeaks - 10)
    textString = "Number of peaks = %d \nreduced chisq = %.2f \nRMSD = %.2f ch (%.2f mm)" % (
        numPeaks,
        reduced_chisq_row,
        rmsd_row,
        rmsd_row_mm,
    )
    pylab.figtext(0.5, 0.2, textString)

    filename = "./plots/" + title + ".png"
    pylab.savefig(filename)
    pylab.clf()

    # ---------- plot column number comparison

    x = WSCOL.readX(i)
    y = WSCOL.readY(i)
    y = np.trim_zeros(y, "b")
    ylen = len(y)
    x = x[0:ylen]
    xy = zip(x, y)
    new_xy = ((a, b) for a, b in xy if b >= 0)
    x, y = zip(*new_xy)
    x = np.array(x)
    y = np.array(y)
    chisq_col = np.sum((x - y) ** 2)
    IDnum = WSCOL.getSpectrum(i).getSpectrumNo()
    title = "bank" + str(IDnum) + "_Col"

    pylab.plot(x, y, "r+")
    pylab.plot(XCALC, YCALC)

    pylab.xlabel("Calculated Column Number")
    pylab.ylabel("Observed Column Number")
    pylab.grid(True)

    pylab.title(title)

    numPeaks = len(x)
    rmsd_col = math.sqrt((1.0 / numPeaks) * chisq_col)
    rmsd_col_mm = rmsd_col * 150 / 256
    reduced_chisq_col = chisq_col / (numPeaks - 10)
    textString = "Number of peaks = %d \nreduced chisq = %.2f ch \nRMSD = %.2f ch (%.2f mm)" % (
        numPeaks,
        reduced_chisq_col,
        rmsd_col,
        rmsd_col_mm,
    )
    pylab.figtext(0.5, 0.2, textString)

    filename = "./plots/" + title + ".png"
    pylab.savefig(filename)
    pylab.clf()

    rmsd_combined = math.sqrt((1.0 / (2.0 * numPeaks)) * (chisq_col + chisq_row))
    rmsd_combined_mm = rmsd_combined * 150 / 256
    OUTPUT.write(" %2d  %8d  %8.2f  %8.2f  %8.2f\n" % (IDnum, numPeaks, rmsd_col_mm, rmsd_row_mm, rmsd_combined_mm))

print("\nAll done!")
