from PyQt4 import QtGui, uic, QtCore
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_pd_sc_conversion_script import PdAndScConversionScript
import ui.inelastic.ui_dgs_pd_sc_conversion

class PdAndScConversionWidget(BaseWidget):
    """
        Widget that presents powder and single crystal data conversion options 
        to the user.
    """ 
    ## Widget name
    name = "Powder and SC"
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(PdAndScConversionWidget, self).__init__(parent, state, settings, 
                                                      data_type=data_type)

        class PdAndScConversionFrame(QtGui.QFrame, ui.inelastic.ui_dgs_pd_sc_conversion.Ui_PdScConversionFrame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = PdAndScConversionFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        self._instrument_name = settings.instrument_name
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(PdAndScConversionScript(self._instrument_name))
    
    def initalize_content(self):
        pass
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: PdAndScConversionScript object
        """    
        pass
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        p = PdAndScConversionScript(self._instrument_name)
        return p
