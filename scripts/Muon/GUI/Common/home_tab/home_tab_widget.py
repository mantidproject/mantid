# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter

from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_model import HomeGroupingWidgetModel
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_view import HomeGroupingWidgetView
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_presenter import HomeGroupingWidgetPresenter

from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter

from Muon.GUI.Common.home_tab.home_tab_model import HomeTabModel
from Muon.GUI.Common.home_tab.home_tab_view import HomeTabView
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabPresenter


class HomeTabWidget(object):
    def __init__(self, context, parent):
        self.inst_view = InstrumentWidgetView(parent)
        self.grp_view = HomeGroupingWidgetView(parent)
        self.run_info_view = HomeRunInfoWidgetView(parent)

        # keep a handle to the presenters of sub-widgets
        self.instrument_widget = InstrumentWidgetPresenter(self.inst_view,
                                                           InstrumentWidgetModel(context=context))
        self.group_widget = HomeGroupingWidgetPresenter(self.grp_view, HomeGroupingWidgetModel(context=context))
        self.run_info_widget = HomeRunInfoWidgetPresenter(self.run_info_view,
                                                          HomeRunInfoWidgetModel(context=context))
        self.home_tab_view = HomeTabView(parent=parent,
                                         widget_list=[self.inst_view,
                                                      self.grp_view,
                                                      self.run_info_view])
        self.home_tab_model = HomeTabModel(context=context)
        self.home_tab_widget = HomeTabPresenter(self.home_tab_view, self.home_tab_model,
                                                subwidgets=[self.instrument_widget,
                                                            self.group_widget,
                                                            self.run_info_widget])

        context.update_view_from_model_notifier.add_subscriber(self.home_tab_widget.update_view_from_model_observer)
