from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter

from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_model import HomeGroupingWidgetModel
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_view import HomeGroupingWidgetView
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_presenter import HomeGroupingWidgetPresenter

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from Muon.GUI.Common.home_plot_widget.home_plot_widget_view import HomePlotWidgetView
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter

from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter

from Muon.GUI.Common.home_tab.home_tab_model import HomeTabModel
from Muon.GUI.Common.home_tab.home_tab_view import HomeTabView
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabPresenter


class HomeTabWidget(object):
    def __init__(self, context):
        self.inst_view = InstrumentWidgetView()
        self.grp_view = HomeGroupingWidgetView()
        self.plot_view = HomePlotWidgetView()
        self.run_info_view = HomeRunInfoWidgetView()

        # keep a handle to the presenters of sub-widgets
        self.instrument_widget = InstrumentWidgetPresenter(self.inst_view,
                                                           InstrumentWidgetModel(muon_data=context))
        self.group_widget = HomeGroupingWidgetPresenter(self.grp_view, HomeGroupingWidgetModel(muon_data=context))
        self.plot_widget = HomePlotWidgetPresenter(self.plot_view, HomePlotWidgetModel())
        self.run_info_widget = HomeRunInfoWidgetPresenter(self.run_info_view,
                                                          HomeRunInfoWidgetModel(muon_data=context))

        self.home_tab_view = HomeTabView(parent=None,
                                         widget_list=[self.inst_view,
                                                      self.grp_view,
                                                      self.plot_view,
                                                      self.run_info_view])
        self.home_tab_model = HomeTabModel(muon_data=context)
        self.home_tab_widget = HomeTabPresenter(self.home_tab_view, self.home_tab_model,
                                                subwidgets=[self.instrument_widget,
                                                            self.group_widget,
                                                            self.plot_widget,
                                                            self.run_info_widget])
