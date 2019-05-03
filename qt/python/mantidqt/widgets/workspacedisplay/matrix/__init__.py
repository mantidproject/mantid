# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.workspacedisplay.matrix.io import MatrixWorkspaceDisplayDecoder, MatrixWorkspaceDisplayEncoder
from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.project.decoderfactory import DecoderFactory


def compatible_check_for_encoder(obj, _):
    return isinstance(obj, StatusBarView) and isinstance(obj.presenter, MatrixWorkspaceDisplay)


EncoderFactory.register_encoder(MatrixWorkspaceDisplayEncoder, compatible_check_for_encoder)
DecoderFactory.register_decoder(MatrixWorkspaceDisplayDecoder)
