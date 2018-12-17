from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter

from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.MuonAnalysis.load_widget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.load_widget.load_widget_presenter import LoadWidgetPresenter

class LoadWidget(object):
    def __init__(self, loaded_data, instrument, parent):
        # set up the views
        self.load_file_view = BrowseFileWidgetView(parent)
        self.load_run_view = LoadRunWidgetView(parent)
        self.load_widget_view = LoadWidgetView(parent=parent,
                                               load_file_view=self.load_file_view,
                                               load_run_view=self.load_run_view)
        self.load_widget = LoadWidgetPresenter(self.load_widget_view,
                                               LoadWidgetModel(loaded_data))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel(loaded_data))
        self.run_widget = LoadRunWidgetPresenter(self.load_run_view, LoadRunWidgetModel(loaded_data))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

        self.load_widget.set_current_instrument(instrument)