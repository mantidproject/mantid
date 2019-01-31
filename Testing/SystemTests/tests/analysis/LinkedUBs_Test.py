# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init
"""
System test that loads two SXD data files and 
links the indexing of their reflections across orientations.
"""
from __future__ import (absolute_import, division, print_function)
import systemtesting
import os
from mantid.simpleapi import *
from mantid.api import FileFinder
from math import sin, cos, pi 
import numpy as np 

class LinkedUBs_Test(systemtesting.MantidSystemTest):

    def cleanup(self):
        Files = ["SXD30904.raw",
                 "SXD30905.raw"]
        for filename in Files:
            absfile = FileFinder.getFullPath(filename)
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
      savedir = os.path.abspath(os.path.curdir)

      ws1 = LoadRaw(Filename='SXD30904.raw', OutputWorkspace='SXD30904')
      ws2 = LoadRaw(Filename='SXD30905.raw', OutputWorkspace='SXD30905')

      UB1 = np.array([[ 0.00099434,  0.17716870, -0.00397909],
                      [ 0.17703120, -0.00117345, -0.00800899],
                      [-0.00803319, -0.00393000, -0.17699037]])

      SetUB(ws1, UB=UB1)
      SetGoniometer(Workspace=ws1, Axis0='-26,0,1,0,-1')
      PredictPeaks(InputWorkspace=ws1, 
          WavelengthMin=0.2, 
          WavelengthMax=10, 
          MinDSpacing=0.5, 
          ReflectionCondition='All-face centred', 
          OutputWorkspace='SXD30905_predict_peaks')

      FindSXPeaks(InputWorkspace=ws2, PeakFindingStrategy='AllPeaks', 
          ResolutionStrategy='AbsoluteResolution', XResolution=0.2, 
          PhiResolution=2, TwoThetaResolution=2, 
          OutputWorkspace='SXD30905_find_peaks')

      # linkedUBs
      LinkedUBs(qTolerance=0.1,
                qDecrement=0.95, 
                dTolerance=0.02, 
                numPeaks=25,
                peakIncrement=10,
                Iterations=10, 
                a=5.6, 
                b=5.6, 
                c=5.6, 
                alpha=90,
                beta=90,
                gamma=90,
                MinWavelength=0.2,
                MaxWavelength=10,
                MinDSpacing=0.5, 
                MaxDSpacing=5
,                ReflectionCondition='All-face centred',
                Workspace=ws2, 
                ObservedPeaks='SXD30905_find_peaks', 
                PredictedPeaks='SXD30905_predict_peaks', 
                LinkedPeaks='SXD30905_linked_peaks', 
                LinkedPredictedPeaks='SXD30905_linked_peaks_predicted',
                DeleteWorkspace=False)

      UB2 = np.array([[ 0.00577285,  0.00610992,  0.17837348],
                      [-0.07735465,  0.16091912, -0.00300855],
                      [-0.16084367, -0.07717164,  0.00784892]])


      linkedUB = mtd['SXD30905_linked_peaks'].sample().getOrientedLattice().getUB()

      diff = linkedUB - UB2 < 0.01
      for c in range(3):
          # This compares each column, allowing old == new OR old == -new
          if not np.all(diff[:,c]):
              raise Exception("More than 0.001 difference between UB matrices: Q (lab frame):\n%s\nQ (sample frame):\n%s" % (linkedUB, UB2))

      def doValidation(self):
        # If we reach here, no validation failed
        print('test successful!')
        return True