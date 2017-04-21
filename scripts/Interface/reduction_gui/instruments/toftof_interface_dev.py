#pylint: disable = R0923
"""
TOFTOF reduction workflow gui.
"""
from reduction_gui.instruments.interface import InstrumentInterface

from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFReductionScripter
from reduction_gui.widgets.toftof.toftof_setup       import TOFTOFSetupWidget

#-------------------------------------------------------------------------------


class TOFTOFInterface(InstrumentInterface):
    ''' The interface for TOFTOF reduction. '''

    def __init__(self, name, settings):
        InstrumentInterface.__init__(self, name, settings)

        self.ERROR_REPORT_NAME   = 'toftof_error_report.xml'
        self.LAST_REDUCTION_NAME = '.mantid_last_toftof_reduction.xml'

        self.scripter = TOFTOFReductionScripter(name, settings.facility_name)
        self.attach(TOFTOFSetupWidget(settings))

#-------------------------------------------------------------------------------
# eof
