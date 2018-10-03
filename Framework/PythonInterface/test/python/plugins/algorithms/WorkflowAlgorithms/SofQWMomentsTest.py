# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import SofQWMoments, CreateSampleWorkspace, ScaleX, RenameWorkspace, LoadInstrument, SofQW
from mantid import mtd

class SofQWMomentsTest(unittest.TestCase):

    def createSampleWorkspace(self):
        """ Create a dummy workspace that looks like a sample run"""
        #create a dummy workspace
        function = "name=Lorentzian,Amplitude=1,PeakCentre=5,FWHM=1"
        ws = CreateSampleWorkspace("Histogram", Function="User Defined", UserDefinedFunction=function, XMin=0, XMax=10, BinWidth=0.01, XUnit="DeltaE")
        ws = ScaleX(ws, -5, "Add") #shift to center on 0
        ws = ScaleX(ws, 0.1) #scale to size
        LoadInstrument(ws, InstrumentName='IRIS', RewriteSpectraMap=True)
        return ws

    def test_sofqwmoments(self):
        ws = self.createSampleWorkspace()

        #Run SofQW and then SofQWMoments
        ws = SofQW(ws, '0.4, 0.1, 1.8', EMode='Indirect', EFixed='1.845')
        ws = SofQWMoments(ws)

        nHists = ws.getNumberHistograms()
        self.assertEquals(nHists, 5)

        self.assertEquals(ws.blocksize(), 14)

if __name__=="__main__":
    unittest.main()
