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

    training_data_dir = '/media/data1/external-data/training-data'
    SXD23767 = Load(Filename=training_data_dir + '/SXD23767.raw', LoadMonitors='Exclude',SpectrumMax=1000)
    mdws = ConvertToMD(InputWorkspace=SXD23767, QDimensions="Q3D",
                       dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
                       LorentzCorrection='1', MinValues=[-15, -15, -15], MaxValues=[15, 15, 15],
                       SplitInto='2', SplitThreshold='50', MaxRecursionDepth='14')
    peaksws = Load(training_data_dir + '/peaks_qLab.nxs')
    app = QApplication([])
    sv = SliceViewer(mdws)
    sv.overlay_peaks_workspace()
    app.exec_()