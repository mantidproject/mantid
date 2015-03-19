#pylint: disable=invalid-name,no-init
"""
System test that loads TOPAZ single-crystal data,
and runs Diffraction Workflow.
"""
import stresstesting
import numpy
from mantid.simpleapi import *
from mantid.api import FileFinder

import os

class Diffraction_Workflow_Test(stresstesting.MantidStressTest):

    def cleanup(self):
        Files = ["TOPAZ_3132.hkl",
        "TOPAZ_3132FFT.hkl"]
        for file in Files:
            absfile = FileFinder.getFullPath(file)
            if os.path.exists(absfile):
                os.remove(absfile)
        return True

    def requiredMemoryMB(self):
        """ Require about 4GB free """
        return 4000

    def runTest(self):
        import platform
        if platform.system() == "Darwin":
            import resource
            # Activate core dumps to try & find the reason for the crashes
            resource.setrlimit(resource.RLIMIT_CORE, (-1, -1))

        # determine where to save
        import os
        savedir = os.path.abspath(os.path.curdir)


        # Basic parameters  for  Triphylite Crystal
        #Name of the workspaces to create
        ws = "TOPAZ_3132"
        filename = ws+"_event.nxs"
        LoadEventNexus(Filename=filename,OutputWorkspace=ws,FilterByTofMin='3000',FilterByTofMax='16000')

        # Spherical Absorption and Lorentz Corrections
        AnvredCorrection(InputWorkspace=ws,OutputWorkspace=ws,LinearScatteringCoef="0.451",LinearAbsorptionCoef="0.993",Radius="0.14")

        # Convert to Q space
        ConvertToDiffractionMDWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_MD2',LorentzCorrection='0',
               OutputDimensions='Q (lab frame)', SplitInto='2',SplitThreshold='150') #,Version=1
        # Find peaks (Reduced number of peaks so file comparison with reference does not fail with small differences)
        FindPeaksMD(InputWorkspace=ws+'_MD2',MaxPeaks='20',OutputWorkspace=ws+'_peaksLattice')
        # 3d integration to centroid peaks
        CentroidPeaksMD(InputWorkspace=ws+'_MD2',CoordinatesToUse='Q (lab frame)',
                PeakRadius='0.12',PeaksWorkspace=ws+'_peaksLattice',OutputWorkspace=ws+'_peaksLattice')
        # Find the UB matrix using the peaks and known lattice parameters
        FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaksLattice',a='10.3522',b='6.0768',c='4.7276',
                       alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
        # And index to HKL
        IndexPeaks(PeaksWorkspace=ws+'_peaksLattice', Tolerance='0.12')
        # Integrate peaks in Q space using spheres
        IntegratePeaksMD(InputWorkspace=ws+'_MD2',PeakRadius='0.12',
                BackgroundOuterRadius='0.18',BackgroundInnerRadius='0.15',
                PeaksWorkspace=ws+'_peaksLattice',OutputWorkspace=ws+'_peaksLattice')
        # Save for SHELX
        SaveHKL(InputWorkspace=ws+'_peaksLattice', Filename=savedir+'/'+ws+'.hkl')

        # Find peaks again for FFT
        FindPeaksMD(InputWorkspace=ws+'_MD2',MaxPeaks='100',OutputWorkspace=ws+'_peaksFFT')
        # 3d integration to centroid peaks
        CentroidPeaksMD(InputWorkspace=ws+'_MD2',       CoordinatesToUse='Q (lab frame)',
                PeakRadius='0.12',PeaksWorkspace=ws+'_peaksFFT',OutputWorkspace=ws+'_peaksFFT')
        # Find the UB matrix using FFT
        FindUBUsingFFT(PeaksWorkspace=ws+'_peaksFFT',MinD=3.,MaxD=14.)

        ## TODO conventional cell

        # And index to HKL
        alg = IndexPeaks(PeaksWorkspace=ws+'_peaksFFT', Tolerance='0.12')

        # Integrate peaks in Q space using spheres
        IntegratePeaksMD(InputWorkspace=ws+'_MD2',PeakRadius='0.12',
                BackgroundOuterRadius='0.18',BackgroundInnerRadius='0.15',
                PeaksWorkspace=ws+'_peaksFFT',OutputWorkspace=ws+'_peaksFFT')
        # Save for SHELX
        SaveHKL(InputWorkspace=ws+'_peaksFFT', Filename=savedir+'/'+ws+'FFT.hkl')


        # Copy the UB matrix back to the original workspace
        CopySample(InputWorkspace=ws+'_peaksFFT',OutputWorkspace=ws,
                       CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0',  CopyLattice=1)
        # Convert to reciprocal space, in the sample frame

        ConvertToDiffractionMDWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_HKL',
                       OutputDimensions='HKL',LorentzCorrection='0', SplitInto='2',SplitThreshold='150')
        # Bin to a regular grid
        BinMD(InputWorkspace=ws+'_HKL',AlignedDim0="[H,0,0], -20, 20, 800",AlignedDim1="[0,K,0], -5, 5, 50",
              AlignedDim2="[0,0,L], -10, 10,  800",OutputWorkspace=ws+'_binned')


        originalUB = numpy.array(mtd["TOPAZ_3132"].sample().getOrientedLattice().getUB())
        w = mtd["TOPAZ_3132"]
        s = w.sample()
        ol = s.getOrientedLattice()
        self.assertDelta( ol.a(), 4.712, 0.01, "Correct lattice a value not found.")
        self.assertDelta( ol.b(), 6.06, 0.01, "Correct lattice b value not found.")
        self.assertDelta( ol.c(), 10.41, 0.01, "Correct lattice c value not found.")
        self.assertDelta( ol.alpha(), 90, 0.4, "Correct lattice angle alpha value not found.")
        self.assertDelta( ol.beta(), 90, 0.4, "Correct lattice angle beta value not found.")
        self.assertDelta( ol.gamma(), 90, 0.4, "Correct lattice angle gamma value not found.")

        # Go to HKL
        ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3132',OutputWorkspace='TOPAZ_3132_HKL',OutputDimensions='HKL',LorentzCorrection='1',SplitInto='2',SplitThreshold='150')


        # Bin to a line (H=0 to 6, L=3, K=3)
        BinMD(InputWorkspace='TOPAZ_3132_HKL',AxisAligned='0',
            BasisVector0='X,units,1,0,0',BasisVector1='Y,units,6.12323e-17,1,0',BasisVector2='2,units,-0,0,1',
            Translation='-0,3,6',OutputExtents='0,6, -0.1,0.1, -0.1,0.1',OutputBins='60,1,1',
            OutputWorkspace='TOPAZ_3132_HKL_line')

        # Now check the integrated bin and the peaks
        w = mtd["TOPAZ_3132_HKL_line"]
        self.assertLessThan( w.signalAt(1), 1e4, "Limited background signal" )
        self.assertDelta( w.signalAt(10), 140.824, 1, "Peak 1") #self.assertDelta( w.signalAt(10), 1110.86, 10, "Peak 1")
        self.assertDelta( w.signalAt(20),  36.25, 1, "Peak 2") #self.assertDelta( w.signalAt(20),  337.71, 10, "Peak 2")
        self.assertDelta( w.signalAt(30),  26.53, 1, "Peak 3") #self.assertDelta( w.signalAt(30),  195.548, 10, "Peak 3")

        # Now do the same peak finding with Q in the sample frame


        ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3132',OutputWorkspace='TOPAZ_3132_QSample',OutputDimensions='Q (sample frame)',LorentzCorrection='1',SplitInto='2',SplitThreshold='150')
        FindPeaksMD(InputWorkspace='TOPAZ_3132_QSample',PeakDistanceThreshold='0.12',MaxPeaks='200',OutputWorkspace='peaks_QSample')
        FindUBUsingFFT(PeaksWorkspace='peaks_QSample',MinD='2',MaxD='16')
        CopySample(InputWorkspace='peaks_QSample',OutputWorkspace='TOPAZ_3132',CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0')

        # Index the peaks and check
        results = IndexPeaks(PeaksWorkspace='peaks_QSample')
        indexed = results[0]
        if indexed < 100:
            raise Exception("Expected at least 100 of 100 peaks to be indexed. Only indexed %d!" % indexed)

        # Check the UB matrix
        w = mtd["TOPAZ_3132"]
        s = w.sample()
        ol = s.getOrientedLattice()
        self.assertDelta( ol.a(), 4.714, 0.01, "Correct lattice a value not found.")
        self.assertDelta( ol.b(), 6.06, 0.01, "Correct lattice b value not found.")
        self.assertDelta( ol.c(), 10.42, 0.01, "Correct lattice c value not found.")
        self.assertDelta( ol.alpha(), 90, 0.4, "Correct lattice angle alpha value not found.")
        self.assertDelta( ol.beta(), 90, 0.4, "Correct lattice angle beta value not found.")
        self.assertDelta( ol.gamma(), 90, 0.4, "Correct lattice angle gamma value not found.")

        # Compare new and old UBs
        newUB = numpy.array(mtd["TOPAZ_3132"].sample().getOrientedLattice().getUB())
        # UB Matrices are not necessarily the same, some of the H,K and/or L sign can be reversed
        diff = abs(newUB) - abs(originalUB) < 0.001
        for c in xrange(3):
            # This compares each column, allowing old == new OR old == -new
            if not numpy.all(diff[:,c]) :
                raise Exception("More than 0.001 difference between UB matrices: Q (lab frame):\n%s\nQ (sample frame):\n%s" % (originalUB, newUB) )

        # load output hkl file and the golden one
        LoadHKL(Filename="TOPAZ_3132.hkl", OutputWorkspace="TOPAZ_3132")
        LoadHKL(Filename='TOPAZ_3132_reference.hkl', OutputWorkspace="TOPAZ_3132_golden")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return ('TOPAZ_3132','TOPAZ_3132_golden')
