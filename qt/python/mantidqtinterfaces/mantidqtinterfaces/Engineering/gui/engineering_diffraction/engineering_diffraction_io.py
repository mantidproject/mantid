# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import logger
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui

import sys

IO_VERSION = 2

SETTINGS_KEYS_TYPES = {
    "save_location": str,
    "full_calibration": str,
    "recalc_vanadium": bool,
    "logs": str,
    "primary_log": str,
    "sort_ascending": bool,
}


class EngineeringDiffractionUIAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    _tags = ["EngineeringDiffractionGui"]


class EngineeringDiffractionEncoder(EngineeringDiffractionUIAttributes):
    def __init__(self):
        super(EngineeringDiffractionEncoder, self).__init__()

    def encode(self, obj: EngineeringDiffractionGui, _=None):
        presenter = obj.presenter
        data_widget = presenter.fitting_presenter.data_widget  # data widget
        plot_widget = presenter.fitting_presenter.plot_widget  # plot presenter
        obj_dic = dict()
        obj_dic["encoder_version"] = IO_VERSION
        obj_dic["current_tab"] = obj.tabs.currentIndex()
        if presenter.settings_presenter.settings:
            obj_dic["settings_dict"] = presenter.settings_presenter.settings
        else:
            obj_dic["settings_dict"] = presenter.settings_presenter.model.get_settings_dict(SETTINGS_KEYS_TYPES)
        if data_widget.model._data_workspaces.get_ws_names_dict():
            obj_dic["data_workspaces"] = data_widget.model._data_workspaces.get_ws_names_dict()
            obj_dic["fit_results"] = plot_widget.model.get_fit_results()
            obj_dic["plotted_workspaces"] = [*data_widget.presenter.plotted]
            if plot_widget.view.fit_browser.read_current_fitprop():
                obj_dic["fit_properties"] = plot_widget.view.fit_browser.read_current_fitprop()
                obj_dic["plot_diff"] = str(plot_widget.view.fit_browser.plotDiff())
            else:
                obj_dic["fit_properties"] = None
        return obj_dic

    @classmethod
    def tags(cls):
        return cls._tags


class EngineeringDiffractionDecoder(EngineeringDiffractionUIAttributes):
    def __init__(self):
        super(EngineeringDiffractionDecoder, self).__init__()

    @staticmethod
    def decode(obj_dic, _=None):
        if obj_dic["encoder_version"] != IO_VERSION:
            logger.error("Engineering Diffraction Interface encoder used different version, restoration may fail")

        ws_names = obj_dic.get("data_workspaces", None)  # workspaces are in ADS, need restoring into interface
        if "workbench" in sys.modules:
            from workbench.config import get_window_config

            parent, flags = get_window_config()
        else:
            parent, flags = None, None
        gui = EngineeringDiffractionGui(parent=parent, window_flags=flags)
        presenter = gui.presenter
        gui.tabs.setCurrentIndex(obj_dic["current_tab"])
        presenter.settings_presenter.model.set_settings_dict(obj_dic["settings_dict"])
        presenter.settings_presenter.settings = obj_dic["settings_dict"]
        if ws_names is not None:
            fit_data_widget = presenter.fitting_presenter.data_widget
            fit_data_widget.model.restore_files(ws_names)
            fit_data_widget.presenter.plotted = set(obj_dic["plotted_workspaces"])
            fit_data_widget.presenter.restore_table(clear_plotted=False)  # Note: clear_plotted=False would avoid
            # presenter.plotted being cleared. Since the slot for presenter._handle_table_cell_changed was defined to
            # use QtCore.Qt.QueuedConnection, during unit tests _handle_table_cell_changed callback is not getting
            # triggered to restore presenter.plotted variable
            fit_plot_widget = presenter.fitting_presenter.plot_widget
            fit_results = obj_dic.get("fit_results", None)
            if fit_results is not None:
                fit_plot_widget.model._fit_results = fit_results
                fit_plot_widget.model.create_fit_tables(
                    fit_data_widget.presenter.get_loaded_ws_list(),
                    fit_data_widget.presenter.get_active_ws_list(),
                    fit_data_widget.presenter.get_log_ws_group_name(),
                )

            fit_properties = obj_dic.get("fit_properties", None)
            if fit_properties is not None:
                fit_browser = fit_plot_widget.view.fit_browser
                fit_browser.show()  # show the fit browser, default is off
                fit_plot_widget.fit_toggle()  # show the fit browser, default is off
                fit_props = fit_properties["properties"]
                fit_function = fit_props["Function"]
                output_name = fit_props["Output"]
                is_plot_diff = obj_dic["plot_diff"]
                fit_browser.setWorkspaceName(output_name)
                fit_browser.setStartX(fit_props["StartX"])
                fit_browser.setEndX(fit_props["EndX"])
                fit_browser.loadFunction(fit_function)
                fit_browser.setOutputName(output_name)
                ws_name = output_name + "_Workspace"
                fit_browser.do_plot(ADS.retrieve(ws_name), is_plot_diff)
        return gui

    @classmethod
    def tags(cls):
        return cls._tags
