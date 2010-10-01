"""
    Base class for instrument-specific user interface
"""
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.reduction.scripter import BaseReductionScripter

class InstrumentInterface(object):
    """
        Defines the instrument-specific widgets
    """
    # Output log for reduction execution
    _output_log = ""
    
    def __init__(self, name, settings):
        """
            Initialization
            @param name: name of the instrument (string)
            @param settings: 
        """
        # Scripter object to interface with Mantid 
        self.scripter = BaseReductionScripter(name=name)
        
        # General settings
        self._settings = settings
        
    def _warning(self, title, message):
        """
            Pop up a dialog and warn the user
            @param title: dialog box title
            @param message: message to be displayed
        """
        return NotImplemented
    
    def load_file(self, file_name):
        """
            Load an XML file containing reduction parameters and
            populate the UI with them
            @param file_name: XML file to be loaded
        """
        #self.scripter = ReductionScripter(name=self.scripter.instrument_name)
        self.scripter.__class__(name=self.scripter.instrument_name)
        self.scripter.from_xml(file_name)
        self._update_from_scripter()
        
    def save_file(self, file_name):
        """
            Save the content of the UI as a settings file that can 
            be reloaded
            @param file_name: XML file to be saved
        """
        self._push_to_scripter()
        self.scripter.to_xml(file_name)
        
    def export(self, file_name):
        """
            Export the content of the UI as a python script that can 
            be run within Mantid
            @param file_name: name of the python script to be saved
        """
        self._push_to_scripter()
        try:
            return self.scripter.to_script(file_name)
        except RuntimeError, e:
            msg = "The following error was encountered:\n\n%s" % unicode(e)
            self._warning("Reduction Parameters Incomplete", msg)
        
    def _push_to_scripter(self):
        """
            Pass the interface data to the scripter
        """
        return NotImplemented
        
    def _update_from_scripter(self):
        """
            Update the interface with the scripter data
        """
        return NotImplemented
    
    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        self._push_to_scripter()
        
        try:
            self._output_log = self.scripter.apply()
            
            # Update widgets
            self._update_from_scripter()
        except:
            msg = "Reduction could not be executed:\n\n%s" % unicode(traceback.format_exc())
            self._warning("Reduction failed", msg)
        
    def get_tabs(self):
        """
            Returns a list of widgets used to populate the central tab widget
            of the interface.
        """
        return []