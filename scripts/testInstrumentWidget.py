from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = InstrumentWidgetPresenter(InstrumentWidgetView(), InstrumentWidgetModel())
    ui.show()
    sys.exit(app.exec_())
