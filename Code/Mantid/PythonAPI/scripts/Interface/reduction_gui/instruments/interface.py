"""
    Base class for instrument-specific user interface
"""
from PyQt4 import QtGui
import traceback
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.reduction.scripter import BaseReductionScripter

class InstrumentInterface(object):
    """
        Defines the instrument-specific widgets
    """
    class WidgetObserverPair(object):
        
        _widget = None
        _observer = None
        
        def __init__(self, widget, observer):
            #TODO: check class types
            self._widget = widget
            self._observer = observer
            
        def widget(self):
            return self._widget
        
        def observer(self):
            return self._observer
        
        def name(self):
            return self._widget.name

    # Output log for reduction execution
    _output_log = ""
    ## List of widgets with associated observers
    widgets = []      
    
    def __init__(self, name, settings):
        """
            Initialization
            @param name: name of the instrument (string)
            @param settings: 
        """
        ## List of widgets with associated observers
        self.widgets = []      

        # Scripter object to interface with Mantid 
        self.scripter = BaseReductionScripter(name=name)
        
        # General settings
        self._settings = settings
        
    def attach(self, widget):
        """
            Attach a widget to the interface and hook it up to its observer/scripter.
            @param widget: QWidget object
        """
        self.widgets.append(InstrumentInterface.WidgetObserverPair(widget, self.scripter.attach(widget)))

    def _warning(self, title, message):
        """
            Pop up a dialog and warn the user
            @param title: dialog box title
            @param message: message to be displayed
            
            #TODO: change this to signals and slots mechanism
        """
        if len(self.widgets)>0:
            QtGui.QMessageBox.warning(self.widgets[0].widget(), title, message)
                      
    
    def load_file(self, file_name):
        """
            Load an XML file containing reduction parameters and
            populate the UI with them
            @param file_name: XML file to be loaded
        """
        #self.scripter = ReductionScripter(name=self.scripter.instrument_name)
        #self.scripter.__class__(name=self.scripter.instrument_name)
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
            return None
        
    def _push_to_scripter(self):
        """
            Pass the interface data to the scripter
        """
        for item in self.widgets:
            item.observer().update()
        
    def _update_from_scripter(self):
        """
            Update the interface with the scripter data
        """
        for item in self.widgets:
            item.widget().set_state(item.observer().state())
    
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
        tab_list = []
        for item in self.widgets:
            tab_list.append([item.name(), item.widget()])
        return tab_list
