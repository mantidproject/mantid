# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
if __name__ == "__main__":
    from mantidqt.widgets.sliceviewer.peaksviewer.view import PeaksViewerCollectionView
    from mantidqt.widgets.sliceviewer.peaksviewer.presenter import PeaksViewerCollectionPresenter
    from mantid.simpleapi import CreatePeaksWorkspace, CreateSampleWorkspace
    from qtpy.QtWidgets import QApplication
    
    sample_ws = CreateSampleWorkspace()
    ws1 = CreatePeaksWorkspace(InstrumentWorkspace=sample_ws, NumberOfPeaks=3)
    ws2 = CreatePeaksWorkspace(InstrumentWorkspace=sample_ws, NumberOfPeaks=5)
    
    app = QApplication([])
    window = PeaksViewerCollectionView()
    presenter = PeaksViewerCollectionPresenter([ws1, ws2], window)
    window.show()
    app.exec_()