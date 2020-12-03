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
    _tags = ["EngineeringDiffractionGUI"]


class EngineeringDiffractionEncoder(EngineeringDiffractionUIAttributes):
    def __init__(self):
        super(EngineeringDiffractionEncoder, self).__init__()

    # TODO
    def encode(self, obj, _=None):  # what is obj? object to encode - > in this case fitting presenter
        data_widget = obj.data_widget
        plot_widget = obj.plot_widget  # plot presenter
        return {"data_loaded_workspaces": data_widget.presenter.get_loaded_workspaces(),
                "fit_properties": plot_widget.read_current_fitprop()}

    @classmethod
    def tags(cls):
        return cls._tags


class EngineeringDiffractionDecoder(EngineeringDiffractionUIAttributes):
    def __init__(self):
        super(EngineeringDiffractionDecoder, self).__init__()

    # TODO
    @staticmethod
    def decode(obj_dic, _=None):
        return EngineeringDiffractionGui(restore_dict=obj_dic)

    @classmethod
    def tags(cls):
        return cls._tags
