from interface import InstrumentInterface

from reduction_gui.widgets.reflectometer.refl_sf_calculator import DataReflSFCalculatorWidget
from reduction_gui.reduction.reflectometer.refl_sf_calculator import REFLSFCalculatorScripter

class REFLSFInterface(InstrumentInterface):
    """
        Defines the widgets for REF_L reduction
    """

    def __init__(self, name, settings):
        super(REFLSFInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "refl_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_refl_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = REFLSFCalculatorScripter(name=name)

        # data REF_L tab
        self.attach(DataReflSFCalculatorWidget(settings = self._settings, name=name))
