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
from mantid.simpleapi import logger
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings

BaseBrowser = import_qt(".._common", "mantidqt.widgets", "FitPropertyBrowser")


class EngDiffFitPropertyBrowser(FitPropertyBrowser):
    """
    A wrapper around python FitPropertyBrowser altered for
    engineering diffraction UI
    """

    def __init__(self, canvas, toolbar_manager, parent=None):
        super(EngDiffFitPropertyBrowser, self).__init__(canvas, toolbar_manager, parent)
        # overwrite default peak with that in settings (gets init when UI opened)
        default_peak = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "default_peak")
        self.setDefaultPeakType(default_peak)
        self.fit_notifier = GenericObservable()
        self.fit_enabled_notifier = GenericObservable()
        self.fit_started_notifier = GenericObservable()
        self.algorithmStarted.connect(self.fitting_started_slot)
        self.algorithmFailed.connect(self.fitting_failed_slot)
        self.function_changed_notifier = GenericObservable()

    def set_output_window_names(self):
        """
         Override this function as no window
        :return: None
        """
        return None

    def get_fitprop(self):
        """
        Get the algorithm parameters updated post-fit (including output status of fit)
        :return: dictionary of parameters
        """
        dict_str = self.getFitAlgorithmParameters()
        if dict_str:
            # evaluate string to make a dict (replace case of bool values)
            fitprop = eval(dict_str.replace("true", "True").replace("false", "False"))
            fitprop["peak_centre_params"] = self._get_center_param_names()
            fitprop["status"] = self.getFitAlgorithmOutputStatus()
            return fitprop
        else:
            # if no fit has been performed
            return None

    def read_current_fitprop(self):
        """
        Get algorithm parameters currently displayed in the UI browser (incl. defaults that user cannot change) which
        will be used as input for the sequential fit. This return does not include the output status of the last fit
        which is not forced to be valid for the current parameters (i.e. could have been changed post-fit)
        :return: dict in style of self.getFitAlgorithmParameters()
        """
        try:
            fitprop = {
                "properties": {
                    "InputWorkspace": self.workspaceName(),
                    "Output": self.outputName(),
                    "StartX": self.startX(),
                    "EndX": self.endX(),
                    "Function": self.getFunctionString(),
                    "ConvolveMembers": True,
                    "OutputCompositeMembers": True,
                }
            }
            exclude = self.getExcludeRange()
            if exclude:
                fitprop["properties"]["Exclude"] = [int(s) for s in exclude.split(",")]
            fitprop["peak_centre_params"] = self._get_center_param_names()
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
        for ax in self.canvas.figure.get_axes():
            try:
                for ws_name, artists in ax.tracked_workspaces.items():
                    self.ws_is_valid(ws_name, True)
            except AttributeError:  # scripted plots have no tracked_workspaces
                pass
        super(EngDiffFitPropertyBrowser, self).show()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())

    def hide(self):
        """
        Override the base class method. Hide the peak editing tool.
        """
        super(EngDiffFitPropertyBrowser, self).hide()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())

    def ws_is_valid(self, ws_name, warn):
        is_valid = ADS.retrieve(ws_name).getAxis(0).getUnit().caption() == "Time-of-flight"
        if not is_valid and warn:
            logger.warning(f"Workspace {ws_name} will not be available for fitting because it doesn't have units of TOF")
        return is_valid

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
                        if self.ws_is_valid(ws_name, False):
                            spectrum_list = [artist.spec_num for artist in artists]
                            allowed_spectra[ws_name] = spectrum_list
            except AttributeError:  # scripted plots have no tracked_workspaces
                pass
        return allowed_spectra

    def _get_center_param_names(self):
        peak_prefixes = self.getPeakPrefixes()
        peak_center_params = []
        for peak_pre in peak_prefixes:
            handler = self.getPeakHandler(peak_pre)
            func_name_prefixed = handler.functionName()
            func_name = func_name_prefixed.split("-")[1]  # separate the prefix from the function name
            peak_center_params.append(func_name + "_" + self.getCentreParameterNameOf(peak_pre))
        return peak_center_params

    @Slot(str)
    def fitting_done_slot(self, name):
        """
        This is called after Fit finishes to update the fit curves.
        :param name: The name of Fit's output workspace.
        """
        super(EngDiffFitPropertyBrowser, self).fitting_done_slot(name)
        self.save_current_setup(self.workspaceName())
        self.fit_notifier.notify_subscribers([self.get_fitprop()])

    @Slot()
    def fitting_failed_slot(self):
        """
        This is called after Fit fails due to an exception thrown.
        """
        self.fit_notifier.notify_subscribers([])

    @Slot()
    def function_changed_slot(self):
        """
        Update the peak editing tool after function structure has changed in
        the browser: functions added and/or removed.
        """
        super(EngDiffFitPropertyBrowser, self).function_changed_slot()
        self.fit_enabled_notifier.notify_subscribers(self.isFitEnabled() and self.isVisible())
        self.function_changed_notifier.notify_subscribers()

    @Slot()
    def fitting_started_slot(self):
        """
        Set the progress bar to in progress.
        # :param name: The name of Fit's input workspace.
        """
        self.fit_started_notifier.notify_subscribers()
