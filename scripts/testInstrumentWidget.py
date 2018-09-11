from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets
from PyQt4 import QtGui

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

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = InstrumentWidgetPresenter(InstrumentWidgetView(), InstrumentWidgetModel())
    ui.show()
    #
    # ui2 = HomeGroupingWidgetPresenter(HomeGroupingWidgetView(), HomeGroupingWidgetModel())
    # ui2.show()
    #
    # ui3 = HomePlotWidgetPresenter(HomePlotWidgetView(), HomePlotWidgetModel())
    # ui3.show()
    #
    # ui4 = HomeRunInfoWidgetPresenter(HomeRunInfoWidgetView(), HomeRunInfoWidgetModel())
    # ui4.show()

    obj = QtGui.QWidget()

    inst_view = InstrumentWidgetView(obj)
    grp_view = HomeGroupingWidgetView(obj)
    plot_view = HomePlotWidgetView(obj)
    run_info_view = HomeRunInfoWidgetView(obj)

    tab_view = HomeTabView(parent=None,
                           instrument_widget=inst_view,
                           grouping_widget=grp_view,
                           plot_widget=plot_view,
                           run_info_widget=run_info_view)
    tab_model = HomeTabModel()

    ui_tab = HomeTabPresenter(tab_view, tab_model)
    ui_tab.show()

    sys.exit(app.exec_())
