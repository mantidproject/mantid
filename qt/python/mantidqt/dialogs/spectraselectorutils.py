# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantidqt.dialogs.spectraselectordialog import SpectraSelectionDialog, SpectraSelection


def get_spectra_selection(workspaces, parent_widget=None, show_colorfill_btn=False, overplot=False):
    """
    Decides whether it is necessary to request user input when asked to
    plot a list of workspaces. The input dialog will only be shown in
    the case where all workspaces have more than 1 spectrum

    :param workspaces: A list of MatrixWorkspaces that will be plotted
    :param parent_widget: An optional parent_widget to use for the input selection dialog
    :param show_colorfill_btn: An optional flag controlling whether the colorfill button should be shown
    :param overplot: An optional flag detailing whether to overplot onto the current figure
    :returns: Either a SpectraSelection object containing the details of workspaces to plot or None indicating
    the request was cancelled
    :raises ValueError: if the workspaces are not of type MatrixWorkspace
    """
    workspaces = SpectraSelectionDialog.get_compatible_workspaces(workspaces)
    single_spectra_ws = [wksp.getNumberHistograms() for wksp in workspaces if wksp.getNumberHistograms() == 1]

    if len(workspaces) == len(single_spectra_ws):
        plottable = []
    else:
        ws_spectra = [{ws.getSpectrum(i).getSpectrumNo() for i in range(ws.getNumberHistograms())} for ws in workspaces]
        plottable = ws_spectra[0]
        # check if there are no common spectra in workspaces
        if len(ws_spectra) > 1:
            for sp_set in ws_spectra[1:]:
                plottable = plottable.intersection(sp_set)

    if len(single_spectra_ws) == len(workspaces) or len(plottable) == 0:
        # At least 1 workspace contains only a single spectrum and these are no
        # common spectra
        selection = SpectraSelection(workspaces)
        selection.wksp_indices = [0]
        return selection
    else:
        selection_dlg = SpectraSelectionDialog(workspaces, parent=parent_widget,
                                               show_colorfill_btn=show_colorfill_btn, overplot=overplot)
        res = selection_dlg.exec_()
        if res == SpectraSelectionDialog.Rejected:
            # cancelled
            return None
        else:
            user_selection = selection_dlg.selection
            if user_selection == 'colorfill':
                return user_selection
            # the dialog should guarantee that only 1 of spectrum/indices is supplied
            assert user_selection.spectra is None or user_selection.wksp_indices is None
            return user_selection
