# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.
from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from mantidqt.widgets.workspacedisplay.table.io import TableWorkspaceDisplayDecoder, TableWorkspaceDisplayEncoder
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay


def compatible_check_for_encoder(obj, _):
    return isinstance(obj, StatusBarView) and isinstance(obj.presenter, TableWorkspaceDisplay)


DecoderFactory.register_decoder(TableWorkspaceDisplayDecoder)
EncoderFactory.register_encoder(TableWorkspaceDisplayEncoder, compatible_check_for_encoder)
