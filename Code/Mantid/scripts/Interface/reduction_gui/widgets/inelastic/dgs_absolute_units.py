from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_absolute_units_script import AbsoluteUnitsScript
import reduction_gui.widgets.util as util
import ui.inelastic.ui_dgs_absolute_units

class AbsoluteUnitsWidget(BaseWidget):
    """
        Widget that presents absolute units normalisation options to the user.
    """ 
    ## Widget name
    name = "Absolute Units"
    
    def __init__(self, parent=None, state=None, settings=None):
        super(AbsoluteUnitsWidget, self).__init__(parent, state, settings)

        class AbsUnitsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_absolute_units.Ui_AbsUnitsFrame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = AbsUnitsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(AbsoluteUnitsScript())

    def initialize_content(self):
        pass
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: AbsoluteUnitsScript object
        """
        self._content.absunits_gb.setChecked(state.do_absolute_units)
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        a = AbsoluteUnitsScript()
        a.do_absolute_units = self._content.absunits_gb.isChecked()
        return a
        
