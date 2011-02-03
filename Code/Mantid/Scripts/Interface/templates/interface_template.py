"""
    Instrument interface class
"""
from interface import InstrumentInterface
from reduction_gui.reduction.%INSTR_NAME_LOWERCASE%_scripter import %INSTR_NAME%Scripter

# Import your widgets here
from reduction_gui.widgets.%INSTR_NAME_LOWERCASE%_script_widget import %INSTR_NAME%ScriptElementWidget

class %INSTR_NAME%Interface(InstrumentInterface):
    """
        Defines the widgets that fill the GUI tabs
    """
    
    def __init__(self, name, settings):
        super(%INSTR_NAME%Interface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = %INSTR_NAME%Scripter(name=name)        

        # Attach your tabs here
        self.attach(%INSTR_NAME%ScriptElementWidget(settings = self._settings))
        