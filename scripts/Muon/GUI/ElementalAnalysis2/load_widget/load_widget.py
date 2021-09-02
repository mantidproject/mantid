# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter

from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA

from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView
from Muon.GUI.ElementalAnalysis2.load_widget.load_widget_presenter import LoadWidgetPresenterEA

from Muon.GUI.ElementalAnalysis2.load_widget.load_models import BrowseFileWidgetModel, LoadWidgetModel, \
    LoadRunWidgetModel


class LoadWidget(object):
    def __init__(self, loaded_data, context, parent):
        # set up the views
        self.load_file_view = BrowseFileWidgetView(parent)
        self.load_file_view.hide_browse()
        self.load_run_view = LoadRunWidgetView(parent)
        self.load_run_view.hide_current_run_button()
        self.load_run_view.hide_instrument_label()
        self.load_widget_view = LoadWidgetView(parent=parent,
                                               load_file_view=self.load_file_view,
                                               load_run_view=self.load_run_view)
        self.load_widget = LoadWidgetPresenterEA(self.load_widget_view,
                                                 LoadWidgetModel(loaded_data, context))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel(loaded_data, context))
        self.run_widget = LoadRunWidgetPresenterEA(self.load_run_view, LoadRunWidgetModel(loaded_data, context))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

        self.load_widget.set_current_instrument(context.data_context.instrument)

        self.run_widget.updated_directory.add_subscriber(self.file_widget.updated_file_path)

    @property
    def view(self):
        return self.load_widget_view
