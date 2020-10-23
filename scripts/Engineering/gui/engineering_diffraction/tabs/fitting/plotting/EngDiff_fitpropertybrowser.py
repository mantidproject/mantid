# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#

from qtpy.QtCore import Slot
from mantidqt.utils.qt import import_qt
from mantidqt.utils.observer_pattern import GenericObservable
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from mantid.api import AnalysisDataService

BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class EngDiffFitPropertyBrowser(FitPropertyBrowser):
    """
    A wrapper around python FitPropertyBrowser altered for
    engineering diffraction UI
    """

    def __init__(self, canvas, toolbar_manager, parent=None):
        super(EngDiffFitPropertyBrowser, self).__init__(canvas, toolbar_manager, parent)
        self.fit_notifier = GenericObservable()

    def set_output_window_names(self):
        """
         Override this function as no window
        :return: None
        """
        return None

    def _get_allowed_spectra(self):
        """
        Get the workspaces and spectra that can be fitted from the
        tracked workspaces.
        """
        allowed_spectra = {}
        output_wsnames = [self.getWorkspaceList().item(ii).text() for ii in range(self.getWorkspaceList().count())]
        for ax in self.canvas.figure.get_axes():
            try:
                for ws_name, artists in ax.tracked_workspaces.items():
                    # don't allow existing output workspaces (fitted curves) to be added
                    if ws_name not in output_wsnames:
                        spectrum_list = [artist.spec_num for artist in artists]
                        allowed_spectra[ws_name] = spectrum_list
            except AttributeError:  # scripted plots have no tracked_workspaces
                pass
        return allowed_spectra

    @Slot(str)
    def fitting_done_slot(self, name):
        """
        This is called after Fit finishes to update the fit curves.
        :param name: The name of Fit's output workspace.
        """

        ws = AnalysisDataService.retrieve(name)
        self.do_plot(ws, plot_diff=self.plotDiff())
        self.fit_result_ws_name = name
        self.saveFunction(self.workspaceName())
        results_dict = eval(self.getFitAlgorithmParameters().replace('true', 'True').replace('false', 'False'))
        self.fit_notifier.notify_subscribers(results_dict)
