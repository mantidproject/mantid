# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService as ADS  # noqa
from Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui


class EngineeringDiffractionUIAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    _tags = ["EngineeringDiffractionGui"]


class EngineeringDiffractionEncoder(EngineeringDiffractionUIAttributes):
    def __init__(self):
        super(EngineeringDiffractionEncoder, self).__init__()

    def encode(self, obj, _=None):  # what obj = EngineeringDiffractionGui
        data_widget = obj.fitting_presenter.data_widget  # data widget
        plot_widget = obj.fitting_presenter.plot_widget  # plot presenter
        obj_dic = dict()
        obj_dic["current_tab"] = obj.tabs.currentIndex()
        if data_widget.presenter.get_loaded_workspaces():
            obj_dic["data_loaded_workspaces"] = [*data_widget.presenter.get_loaded_workspaces().keys()]
            obj_dic["plotted_workspaces"] = [*data_widget.presenter.plotted]
            if plot_widget.view.fit_browser.get_fitprop():
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
        ws_names = obj_dic["data_loaded_workspaces"]  # workspaces are in ADS, need restoring into interface
        gui = EngineeringDiffractionGui()
        gui.tabs.setCurrentIndex(obj_dic["current_tab"])
        gui.fitting_presenter.data_widget.model.restore_files(ws_names)
        gui.fitting_presenter.data_widget.presenter.plotted = set(obj_dic["plotted_workspaces"])
        gui.fitting_presenter.data_widget.presenter.restore_table()

        if obj_dic["fit_properties"]:
            fit_browser = gui.fitting_presenter.plot_widget.view.fit_browser
            gui.fitting_presenter.plot_widget.view.fit_toggle()  # show the fit browser, default is off
            fit_props = obj_dic["fit_properties"]["properties"]
            fit_function = fit_props["Function"]
            output_name = fit_props["Output"]
            is_plot_diff = obj_dic["plot_diff"]
            fit_browser.setStartX(fit_props["StartX"])
            fit_browser.setEndX(fit_props["EndX"])
            fit_browser.loadFunction(fit_function)
            fit_browser.setOutputName(output_name)
            ws_name = output_name + '_Workspace'
            fit_browser.do_plot(ADS.retrieve(ws_name), is_plot_diff)
        return gui

    @classmethod
    def tags(cls):
        return cls._tags
