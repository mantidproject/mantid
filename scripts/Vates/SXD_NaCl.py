from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


def reportUnitCell(peaks_ws):
    latt = peaks_ws.sample().getOrientedLattice()
    print("-- Unit Cell --")
    print(latt.a())
    print(latt.b())
    print(latt.c())
    print(latt.alpha())
    print(latt.beta())
    print(latt.gamma())


#
# Exclude the monitors when loading the raw SXD file.  This avoids
#
mantid.Load(Filename='SXD23767.raw', OutputWorkspace='SXD23767', LoadMonitors='Exclude')

#
# A lower SplitThreshold, with a reasonable bound on the recursion depth, helps find weaker peaks at higher Q.
#
QLab = mantid.ConvertToDiffractionMDWorkspace(InputWorkspace='SXD23767', OutputDimensions='Q (lab frame)',
                                              SplitThreshold=50, LorentzCorrection='1',
                                              MaxRecursionDepth='13', Extents='-15,15,-15,15,-15,15')

#
#  NaCl has a relatively small unit cell, so the distance between peaks is relatively large.  Setting the PeakDistanceThreshold
#  higher avoids finding high count regions on the sides of strong peaks as separate peaks.
#
peaks_qLab = mantid.FindPeaksMD(InputWorkspace='QLab', MaxPeaks=300, DensityThresholdFactor=10,
                                PeakDistanceThreshold=1.0)

#
#  Fewer peaks index if Centroiding is used.  This indicates that there may be an error in the centroiding algorithm,
#  since the peaks seem to be less accurate.
#
# peaks_qLab = CentroidPeaksMD(InputWorkspace='QLab',PeaksWorkspace=peaks_qLab)

use_fft = True
use_cubic_lat_par = False
use_Niggli_lat_par = False

#
# Note: Reduced tolerance on  FindUBUsingFFT will omit peaks not near the lattice.  This seems to help
# find the Niggli cell correctly, with all angle 60 degrees, and all sides 3.99
#
if use_fft:
    mantid.FindUBUsingFFT(PeaksWorkspace=peaks_qLab, MinD='3', MaxD='5', Tolerance=0.08)
    print('\nNiggli cell found from FindUBUsingFFT:')

if use_cubic_lat_par:
    mantid.FindUBUsingLatticeParameters(PeaksWorkspace=peaks_qLab, a=5.6402, b=5.6402, c=5.6402,
                                        alpha=90, beta=90, gamma=90, NumInitial=25, Tolerance=0.12)
    print('\nCubic cell found directly from FindUBUsingLatticeParameters')

if use_Niggli_lat_par:
    mantid.FindUBUsingLatticeParameters(PeaksWorkspace=peaks_qLab, a=3.9882, b=3.9882, c=3.9882,
                                        alpha=60, beta=60, gamma=60, NumInitial=25, Tolerance=0.12)
    print('\nNiggli cell found from FindUBUsingLatticeParameters:')

reportUnitCell(peaks_qLab)

mantid.IndexPeaks(PeaksWorkspace=peaks_qLab, Tolerance=0.12, RoundHKLs=1)

if use_fft or use_Niggli_lat_par:
    mantid.ShowPossibleCells(PeaksWorkspace=peaks_qLab, MaxScalarError='0.5')
    mantid.SelectCellOfType(PeaksWorkspace=peaks_qLab, CellType='Cubic', Centering='F', Apply=True)

peaks_qLab_Integrated = mantid.IntegratePeaksMD(InputWorkspace=QLab, PeaksWorkspace=peaks_qLab, PeakRadius=0.2,
                                                BackgroundInnerRadius=0.3, BackgroundOuterRadius=0.4)

binned = mantid.BinMD(InputWorkspace=QLab, AlignedDim0='Q_lab_x,-15,15,200', AlignedDim1='Q_lab_y,-15,15,200',
                      AlignedDim2='Q_lab_z,-15,15,200')

print('The final result is:')
reportUnitCell(peaks_qLab)
