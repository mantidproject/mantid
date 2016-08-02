from interface import InstrumentInterface

from reduction_gui.widgets.toftof.toftof_setup import TOFTOFSetupWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, TOFTOFReductionScripter

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------

class TOFTOFInterface(InstrumentInterface):
    ''' The interface for TOFTOF reduction. '''

    def __init__(self, name, settings):
        InstrumentInterface.__init__(self, name, settings)

        self.ERROR_REPORT_NAME   = '%s_error_report.xml' % name
        self.LAST_REDUCTION_NAME = '.mantid_last_%s_reduction.xml' % name

        self.scriptElement = TOFTOFScriptElement()

        self.scripter = TOFTOFReductionScripter(name, settings.facility_name, self.scriptElement)
        self.attach(TOFTOFSetupWidget(settings, self.scriptElement))

#-------------------------------------------------------------------------------
# eof
