import stresstesting
from mantid.simpleapi import *

class SXDAnalysis(stresstesting.MantidStressTest):
    """
    Start of a system test for SXD data analyiss
    """
    
    def runTest(self):
        
        ws = Load(Filename='SXD23767.raw', LoadMonitors='Exclude')
        #AddSampleLog(Workspace=ws,LogName='NUM_THREADS',LogText='0',LogType='Number')
        from time import clock
        
        # A lower SplitThreshold, with a reasonable bound on the recursion depth, helps find weaker peaks at higher Q.
        start = clock();
        QLab = ConvertToDiffractionMDWorkspace(InputWorkspace=ws, OutputDimensions='Q (lab frame)', SplitThreshold=50, LorentzCorrection='1',MaxRecursionDepth='13',Extents='-15,15,-15,15,-15,15',OneEventPerBin='0')
        print " ConvertToMD runs for: ",clock()-start,' sec'
        
        #  NaCl has a relatively small unit cell, so the distance between peaks is relatively large.  Setting the PeakDistanceThreshold
        #  higher avoids finding high count regions on the sides of strong peaks as separate peaks.
        peaks_qLab = FindPeaksMD(InputWorkspace='QLab', MaxPeaks=300, DensityThresholdFactor=10, PeakDistanceThreshold=1.0)

        FindUBUsingFFT(PeaksWorkspace=peaks_qLab, MinD='3', MaxD='5',Tolerance=0.08)
        
        out_params = IndexPeaks(PeaksWorkspace=peaks_qLab,Tolerance=0.12,RoundHKLs=1)
        number_peaks_indexed = out_params[0]
        ratio_indexed = float(number_peaks_indexed)/peaks_qLab.getNumberPeaks()
        self.assertTrue(ratio_indexed >= 0.8, "Not enough peaks indexed. Ratio indexed : " + str(ratio_indexed))
        
        ShowPossibleCells(PeaksWorkspace=peaks_qLab,MaxScalarError='0.5')
        SelectCellOfType(PeaksWorkspace=peaks_qLab, CellType='Cubic', Centering='F', Apply=True)
        
        unitcell_length = 5.64 # Angstroms
        unitcell_angle = 90
        length_tolerance = 0.1
        #
        angle_tolelerance = 0.25  # Actual tolernce seems is 0.17
        #
        # Check results.
        latt = peaks_qLab.sample().getOrientedLattice()
        self.assertDelta( latt.a(), unitcell_length, length_tolerance, "a length is different from expected")
        self.assertDelta( latt.b(), unitcell_length, length_tolerance, "b length is different from expected")
        self.assertDelta( latt.c(), unitcell_length, length_tolerance, "c length is different from expected")
        self.assertDelta( latt.alpha(), unitcell_angle, angle_tolelerance, "alpha angle is different from expected")
        self.assertDelta( latt.beta(), unitcell_angle, angle_tolelerance, "beta angle is different from expected")
        self.assertDelta( latt.gamma(), unitcell_angle, angle_tolelerance, "gamma angle length is different from expected")
        
    def doValidation(self):
        # If we reach here, no validation failed
        return True
    def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 2Gb"""
      return 1000
