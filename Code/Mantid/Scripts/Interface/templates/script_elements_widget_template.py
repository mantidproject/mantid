from PyQt4 import QtGui, uic, QtCore
from reduction_gui.reduction.%INSTR_NAME_LOWERCASE%_script_elements import %INSTR_NAME%ScriptElement
from base_widget import BaseWidget

# You need to import your widget definition here (usually generated with QT Designer)
#import ui.ui_%INSTR_NAME_LOWERCASE%_script_element_widget

class %INSTR_NAME%ScriptElementWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    ## Widget name
    name = "%INSTR_NAME%ScriptElement"      
    
    def __init__(self, parent=None, state=None, settings=None, show_transmission=True):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        # The following is how you use the widget defined in your .ui file
        #class %INSTR_NAME%ScriptFrame(QtGui.QFrame, ui.ui_%INSTR_NAME_LOWERCASE%_script_element_widget): 
        #    def __init__(self, parent=None):
        #        QtGui.QFrame.__init__(self, parent)
        #        self.setupUi(self)
        
        # Dummy version
        class %INSTR_NAME%ScriptFrame(QtGui.QFrame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
        
        self._content = %INSTR_NAME%ScriptFrame(self)
        
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(%INSTR_NAME%ScriptElement())
            
        self._last_direct_state = None
        self._last_spreader_state = None

    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        pass
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: %INSTR_NAME%ScriptElement object
        """
        pass
        
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = %INSTR_NAME%ScriptElement()  
        # Modify the created object according to UI content
        return m
        