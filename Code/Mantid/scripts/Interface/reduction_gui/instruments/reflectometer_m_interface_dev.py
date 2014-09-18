from interface import InstrumentInterface
from reduction_gui.widgets.reflectometer.refm_reduction import DataReflWidget
from reduction_gui.widgets.output import OutputWidget
try:
    from reduction_gui.widgets.reflectometer.stitcher import StitcherWidget
    HAS_STITCHER = True
except:
    HAS_STITCHER = False

class REFMInterface(InstrumentInterface):
    """
        Defines the widgets for REF_M reduction
    """

    def __init__(self, name, settings):
        super(REFMInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "refm_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_refm_reduction.xml"

        # data REF_M tab
        self.attach(DataReflWidget(settings = self._settings, name=name))
        # Reduction output
        self.attach(OutputWidget(settings = self._settings))
        if HAS_STITCHER:
            self.attach(StitcherWidget(settings = self._settings))
