# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
"""
You can run this widget independently by for example:

    from mantidqt.widgets.instrumentview.presenter import InstrumentView
    from mantid.simpleapi import Load
    from qtpy.QtWidgets import QApplication

    ws=Load('CNCS_7860')

    app = QApplication([])
    window = InstrumentView(ws)
    app.exec_()
"""
from __future__ import (absolute_import, unicode_literals)

# 3rdparty imports
from qtpy import PYQT4

from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.widgets.instrumentview.io import InstrumentViewDecoder, InstrumentViewEncoder

if PYQT4:
    raise ImportError("Instrument view requires Qt >= v5")


# Add encoder and decoders to the relevant factory
DecoderFactory.register_decoder(InstrumentViewDecoder)
EncoderFactory.register_encoder(InstrumentViewEncoder)
