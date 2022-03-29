# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
project Save for Reduction GUI for DNS Instrument at MLZ
"""
from mantidqtinterfaces.dns.main_widget import DNSReductionGuiWidget


class InterfaceAttributes():
    # WARNING: If you delete a tag from here instead of adding a new one,
    # it will make old project files obsolete so
    # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
    # This list must contain the name of the class that will be found at the
    # top level of Widgets, this is usually the view
    # class
    _tags = ["DNSReductionGuiView"]


class DNSReductionGuiWidgetEncoder(InterfaceAttributes):

    def encode(self, obj, project_path=None):
        # pylint: disable=unused-argument, no-self-use
        widget = obj.parent
        return widget.parameter_abo.get_gui_param()

    @classmethod
    def tags(cls):
        return cls._tags


class DNSReductionGuiWidgetDecoder(InterfaceAttributes):

    def decode(self, obj_dict, project_path=None):
        # pylint: disable=unused-argument, no-self-use
        # Recreate the GUI in a base state
        widget = DNSReductionGuiWidget(name='DNS-Reduction')
        # Restore the state from the dictionary
        widget.parameter_abo.project_save_load(obj_dict)
        # Return the view of the GUI or whatever object can have .show()
        # called on it
        return widget.view

    @classmethod
    def tags(cls):
        return cls._tags
