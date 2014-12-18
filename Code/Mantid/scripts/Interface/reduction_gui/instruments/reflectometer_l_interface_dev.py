from interface import InstrumentInterface

from reduction_gui.widgets.reflectometer.refl_reduction import DataReflWidget
from reduction_gui.reduction.reflectometer.refl_reduction import REFLReductionScripter
try:
    from reduction_gui.widgets.reflectometer.stitcher import StitcherWidget
    HAS_STITCHER = True
except:
    HAS_STITCHER = False

class REFLInterface(InstrumentInterface):
    """
        Defines the widgets for REF_L reduction
    """

    def __init__(self, name, settings):
        super(REFLInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "refl_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_refl_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = REFLReductionScripter(name=name)

        # data REF_L tab
        self.attach(DataReflWidget(settings = self._settings, name=name))
        if HAS_STITCHER:
            self.attach(StitcherWidget(settings = self._settings))
