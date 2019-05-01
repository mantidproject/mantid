from mantidqt.dialogs.spectraselectordialog import SpectraSelectionDialog, SpectraSelection


def get_spectra_selection(workspaces, parent_widget=None):
    """Decides whether it is necessary to request user input	
    when asked to plot a list of workspaces. The input	
    dialog will only be shown in the case where all workspaces	
    have more than 1 spectrum	
     :param workspaces: A list of MatrixWorkspaces that will be plotted	
    :param parent_widget: An optional parent_widget to use for the input selection dialog	
    :returns: Either a SpectraSelection object containing the details of workspaces to plot or None indicating	
    the request was cancelled	
    :raises ValueError: if the workspaces are not of type MatrixWorkspace	
    """
    SpectraSelectionDialog.raise_error_if_workspaces_not_compatible(workspaces)
    single_spectra_ws = [wksp.getNumberHistograms() for wksp in workspaces if wksp.getNumberHistograms() == 1]
    if len(single_spectra_ws) > 0:
        # At least 1 workspace contains only a single spectrum so this is all
        # that is possible to plot for all of them
        selection = SpectraSelection(workspaces)
        selection.wksp_indices = [0]
        return selection
    else:
        selection_dlg = SpectraSelectionDialog(workspaces, parent=parent_widget)
        res = selection_dlg.exec_()
        if res == SpectraSelectionDialog.Rejected:
            # cancelled
            return None
        else:
            user_selection = selection_dlg.selection
            # the dialog should guarantee that only 1 of spectrum/indices is supplied
            assert user_selection.spectra is None or user_selection.wksp_indices is None
            return user_selection