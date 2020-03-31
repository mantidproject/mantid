# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# Can be launch from python by _e.g_
#
#
# from mantidqt.widgets.sliceviewer.presenter import SliceViewer
# from mantid.simpleapi import LoadMD
# from qtpy.QtWidgets import QApplication
# ws = LoadMD('ExternalData/Testing/SystemTests/tests/analysis/reference/ConvertWANDSCDtoQTest_HKL.nxs')
# app = QApplication([])
# window = SliceViewer(ws)
# app.exec_()
if __name__ == "__main__":
    from mantidqt.widgets.sliceviewer.presenter import SliceViewer
    from mantid.simpleapi import *
    from qtpy.QtWidgets import QApplication

    SXD23767 = Load(Filename='SXD23767.raw', LoadMonitors='Exclude', SpectrumMax=1000)
    mdws = ConvertToMD(
        InputWorkspace=SXD23767,
        QDimensions="Q3D",
        dEAnalysisMode="Elastic",
        QConversionScales="Q in A^-1",
        LorentzCorrection='1',
        MinValues=[-15, -15, -15],
        MaxValues=[15, 15, 15],
        SplitInto='2',
        SplitThreshold='50',
        MaxRecursionDepth='14')
    peaksws = Load('peaks_qLab.nxs')
    while peaksws.getNumberPeaks() > 5:
        DeleteTableRows(peaksws, Rows=peaksws.getNumberPeaks() - 1)
    IntegratePeaksMD(
        InputWorkspace=mdws,
        PeaksWorkspace=peaksws,
        PeakRadius=0.5,
        OutputWorkspace='peaksws_sphere_nobkgd')
    IntegratePeaksMD(
        InputWorkspace=mdws,
        PeaksWorkspace=peaksws,
        PeakRadius=0.5,
        BackgroundInnerRadius=1.1,
        BackgroundOuterRadius=1.4,
        OutputWorkspace='peaksws_sphere_withbkgd')

    FindUBUsingFFT(PeaksWorkspace=peaksws, MinD=0.8, MaxD=10)
    IndexPeaks(PeaksWorkspace=peaksws, Tolerance=0.1)
    IntegrateEllipsoids(
        InputWorkspace=SXD23767,
        PeaksWorkspace='peaksws',
        RegionRadius=0.5,
        OutputWorkspace='peaksws_ellipse_no_bkgd')

    app = QApplication([])
    sv = SliceViewer(mdws)
    app.exec_()
