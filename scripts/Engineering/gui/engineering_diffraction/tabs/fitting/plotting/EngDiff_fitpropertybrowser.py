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
from mantid.api import AnalysisDataService as ADS

BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class EngDiffFitPropertyBrowser(FitPropertyBrowser):
    """
    A wrapper around python FitPropertyBrowser altered for
    engineering diffraction UI
    """
    def __init__(self, canvas, toolbar_manager, parent=None):
        super(EngDiffFitPropertyBrowser, self).__init__(canvas, toolbar_manager, parent)
        self.fit_notifier = GenericObservable()
        self.fit_enabled_notifier = GenericObservable()

    def set_output_window_names(self):
        """
         Override this function as no window
        :return: None
        """
        return None

    def get_fitprop(self):
        """
        Get the algorithm parameters updated post-fit
        :return: dictionary of parameters
        """
        dict_str = self.getFitAlgorithmParameters()
        if dict_str:
            # evaluate string to make a dict (replace case of bool values)
            return eval(dict_str.replace('true', 'True').replace('false', 'False'))
        else:
            # if no fit has been performed
            return None

    def read_current_fitprop(self):
        """
        Get algorithm parameters currently displayed in the UI browser (incl. defaults that user cannot change)
        :return: dict in style of self.getFitAlgorithmParameters()
        """
        try:
            fitprop = {
                'properties': {
                    'InputWorkspace': self.workspaceName(),
                    'Output': self.outputName(),
                    'StartX': self.startX(),
                    'EndX': self.endX(),
                    'Function': self.getFunctionString(),
                    'ConvolveMembers': True,
                    'OutputCompositeMembers': True
                }
            }
            exclude = self.getExcludeRange()
            if exclude:
                fitprop['properties']['Exclude'] = [int(s) for s in exclude.split(',')]
            return fitprop
        except BaseException:  # The cpp passes up an 'unknown' error if getFunctionString() fails, i.e. if no fit
            return None

    def save_current_setup(self, name):
        self.executeCustomSetupRemove(name)
        self.saveFunction(name)

    def show(self):
        """
        Override the base class method. Hide the peak editing tool.
        """
        super(EngDiffFitPropertyBrowser, self).show()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())

    def hide(self):
        """
        Override the base class method. Hide the peak editing tool.
        """
        super(EngDiffFitPropertyBrowser, self).hide()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())

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
                    if ws_name not in output_wsnames and \
                            ADS.retrieve(ws_name).getAxis(0).getUnit().caption() == 'Time-of-flight':
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
        ws = ADS.retrieve(name)
        self.do_plot(ws, plot_diff=self.plotDiff())
        self.fit_result_ws_name = name
        self.save_current_setup(self.workspaceName())
        peak_prefixes = self.getPeakPrefixes()
        peak_center_params = []
        for peak_pre in peak_prefixes:
            handler = self.getPeakHandler(peak_pre)
            func_name_prefixed = handler.functionName()
            func_name = func_name_prefixed.split('-')[1]  # separate the prefix from the function name
            peak_center_params.append(func_name + '_' + self.getCentreParameterNameOf(peak_pre))
        self.fit_notifier.notify_subscribers([self.get_fitprop(), peak_center_params])  # needs to be passed a list

    @Slot()
    def function_changed_slot(self):
        """
        Update the peak editing tool after function structure has changed in
        the browser: functions added and/or removed.
        """
        super(EngDiffFitPropertyBrowser, self).function_changed_slot()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())
