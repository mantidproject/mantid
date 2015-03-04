"""
System test that loads TOPAZ single-crystal data,
converts to Q space, finds peaks and indexes
them.
"""
import stresstesting
import numpy
from mantid.simpleapi import *

class TOPAZPeakFinding(stresstesting.MantidStressTest):
    
    def requiredMemoryMB(self):
        """ Require about 2GB free """
        return 2000
    
    def runTest(self):
        # Load then convert to Q in the lab frame
        LoadEventNexus(Filename=r'TOPAZ_3132_event.nxs',OutputWorkspace='topaz_3132')
        ConvertToDiffractionMDWorkspace(InputWorkspace='topaz_3132',OutputWorkspace='topaz_3132_MD',LorentzCorrection='1',SplitInto='2',SplitThreshold='150',OneEventPerBin='0')
        
        # Find peaks and UB matrix
        FindPeaksMD(InputWorkspace='topaz_3132_MD',PeakDistanceThreshold='0.12',MaxPeaks='200',OutputWorkspace='peaks')
        FindUBUsingFFT(PeaksWorkspace='peaks',MinD='2',MaxD='16')

        # Index the peaks and check        
        results = IndexPeaks(PeaksWorkspace='peaks')
        indexed = results[0]
        if indexed < 199:
            raise Exception("Expected at least 199 of 200 peaks to be indexed. Only indexed %d!" % indexed)

        # Check the oriented lattice
        CopySample(InputWorkspace='peaks',OutputWorkspace='topaz_3132',CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0')
        originalUB = numpy.array(mtd["topaz_3132"].sample().getOrientedLattice().getUB())
        w = mtd["topaz_3132"]
        s = w.sample()
        ol = s.getOrientedLattice()
        self.assertDelta( ol.a(), 4.712, 0.01, "Correct lattice a value not found.")
        self.assertDelta( ol.b(), 6.06, 0.01, "Correct lattice b value not found.")
        self.assertDelta( ol.c(), 10.41, 0.01, "Correct lattice c value not found.")
        self.assertDelta( ol.alpha(), 90, 0.4, "Correct lattice angle alpha value not found.")
        self.assertDelta( ol.beta(), 90, 0.4, "Correct lattice angle beta value not found.")
        self.assertDelta( ol.gamma(), 90, 0.4, "Correct lattice angle gamma value not found.")
        
        # Go to HKL
        ConvertToDiffractionMDWorkspace(InputWorkspace='topaz_3132',OutputWorkspace='topaz_3132_HKL',OutputDimensions='HKL',LorentzCorrection='1',SplitInto='2',SplitThreshold='150')

        # Bin to a line (H=0 to 6, L=3, K=3)
        BinMD(InputWorkspace='topaz_3132_HKL',AxisAligned='0',
            BasisVector0='X,units,1,0,0',BasisVector1='Y,units,6.12323e-17,1,0',BasisVector2='2,units,-0,0,1',
            Translation='-0,3,6',OutputExtents='0,6, -0.1,0.1, -0.1,0.1',OutputBins='60,1,1',
            OutputWorkspace='topaz_3132_HKL_line')
              
        # Now check the integrated bin and the peaks
        w = mtd["topaz_3132_HKL_line"]
        self.assertLessThan( w.signalAt(1), 1e4, "Limited background signal" )
        # The following tests are unstable for flips in HKL:
        #self.assertDelta( w.signalAt(10), 1043651, 10e3, "Peak 1")
        #self.assertDelta( w.signalAt(20),  354159, 10e3, "Peak 2")
        #self.assertDelta( w.signalAt(30),  231615, 10e3, "Peak 3")

        # Now do the same peak finding with Q in the sample frame
        ConvertToDiffractionMDWorkspace(InputWorkspace='topaz_3132',OutputWorkspace='topaz_3132_QSample',OutputDimensions='Q (sample frame)',LorentzCorrection='1',SplitInto='2',SplitThreshold='150')
        FindPeaksMD(InputWorkspace='topaz_3132_QSample',PeakDistanceThreshold='0.12',MaxPeaks='200',OutputWorkspace='peaks_QSample')
        FindUBUsingFFT(PeaksWorkspace='peaks_QSample',MinD='2',MaxD='16')
        CopySample(InputWorkspace='peaks_QSample',OutputWorkspace='topaz_3132',CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0')
        
        # Index the peaks and check        
        results = IndexPeaks(PeaksWorkspace='peaks_QSample')
        indexed = results[0]
        if indexed < 199:
            raise Exception("Expected at least 199 of 200 peaks to be indexed. Only indexed %d!" % indexed)

        # Check the UB matrix
        w = mtd["topaz_3132"]
        s = w.sample()
        ol = s.getOrientedLattice()
        self.assertDelta( ol.a(), 4.714, 0.01, "Correct lattice a value not found.")
        self.assertDelta( ol.b(), 6.06, 0.01, "Correct lattice b value not found.")
        self.assertDelta( ol.c(), 10.42, 0.01, "Correct lattice c value not found.")
        self.assertDelta( ol.alpha(), 90, 0.4, "Correct lattice angle alpha value not found.")
        self.assertDelta( ol.beta(), 90, 0.4, "Correct lattice angle beta value not found.")
        self.assertDelta( ol.gamma(), 90, 0.4, "Correct lattice angle gamma value not found.")
        
        # Compare new and old UBs
        newUB = numpy.array(mtd["topaz_3132"].sample().getOrientedLattice().getUB())
        # UB Matrices are not necessarily the same, some of the H,K and/or L sign can be reversed
        diff = abs(newUB) - abs(originalUB) < 0.001
        for c in xrange(3):
            # This compares each column, allowing old == new OR old == -new
            if not (numpy.all(diff[:,c]) ):
                raise Exception("More than 0.001 difference between UB matrices: Q (lab frame):\n%s\nQ (sample frame):\n%s" % (originalUB, newUB) )

    def doValidation(self):
        # If we reach here, no validation failed
        return True
