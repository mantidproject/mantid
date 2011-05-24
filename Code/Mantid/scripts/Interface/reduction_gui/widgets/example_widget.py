from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
from reduction_gui.widgets.base_widget import BaseWidget

# Import the reduction script class corresponding to this widget
from reduction_gui.reduction.example_state import ExampleState
import ui.ui_example

class ExampleWidget(BaseWidget):    

    ## Widget name
    name = "Blah"      
    ## Alternate text place holder
    _alternate_text = ''
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """
            @param state: ExampleState object
            @param settings: GeneralSettings object
            @param data_type: String representing the allowed extensions
        """
        super(ExampleWidget, self).__init__(parent, state, settings, data_type) 

        class ExFrame(QtGui.QFrame, ui.ui_example.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = ExFrame(self)
        self._layout.addWidget(self._content)

        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(ExampleState())
            
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        self.connect(self._content.button, QtCore.SIGNAL("clicked()"), self._button_pressed)
        
    def _button_pressed(self):
        _tmp_str = self._alternate_text
        self._alternate_text = self._content.line_edit.text()
        self._content.line_edit.setText(QtCore.QString(_tmp_str))
        
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.line_edit.setText(state.text)
        self._alternate_text = state.alternate_text
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = ExampleState()
        m.text = self._content.line_edit.text()
        m.alternate_text = self._alternate_text
        return m