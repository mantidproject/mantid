"""
    Base class for instrument-specific user interface
"""
from PyQt4 import QtGui
import sys
import os
import traceback
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.reduction.scripter import BaseReductionScripter

class InstrumentInterface(object):
    """
        Defines the instrument-specific widgets
    """
    ## List of widgets with associated observers
    widgets = []  
    ERROR_REPORT_NAME = "sans_error_report.xml"    
    
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
        self.widgets.append(widget)
        self.scripter.attach(widget)

    def destroy(self):
        """
            Destroys all the widget owner by this interface
        """
        for i in range(len(self.widgets)):
            self.widgets.pop().destroy()
        self.scripter.clear()
            
    def _warning(self, title, message):
        """
            Pop up a dialog and warn the user
            @param title: dialog box title
            @param message: message to be displayed
            
            #TODO: change this to signals and slots mechanism
        """
        if len(self.widgets)>0:
            QtGui.QMessageBox.warning(self.widgets[0], title, message)
                      
    
    def load_file(self, file_name):
        """
            Load an XML file containing reduction parameters and
            populate the UI with them
            @param file_name: XML file to be loaded
        """
        #self.scripter = ReductionScripter(name=self.scripter.instrument_name)
        #self.scripter.__class__(name=self.scripter.instrument_name)
        self.scripter.from_xml(file_name)
        self.scripter.push_state()
        
    def save_file(self, file_name):
        """
            Save the content of the UI as a settings file that can 
            be reloaded
            @param file_name: XML file to be saved
        """
        self.scripter.update()
        self.scripter.to_xml(file_name)
        
    def export(self, file_name):
        """
            Export the content of the UI as a python script that can 
            be run within Mantid
            @param file_name: name of the python script to be saved
        """
        self.scripter.update()
        try:
            return self.scripter.to_script(file_name)
        except RuntimeError, e:
            if self._settings.debug:
                msg = "The following error was encountered:\n\n%s" % unicode(traceback.format_exc())
            else:
                msg = "The following error was encountered:\n\n%s" % unicode(e)
                log_path = os.path.join(self._settings.data_path, self.ERROR_REPORT_NAME)
                msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction Parameters Incomplete", msg)
            self._error_report(traceback.format_exc())
            return None
        except:
            msg = "The following error was encountered:\n\n%s" % sys.exc_info()[0]
            msg += "\n\nPlease check your reduction parameters\n"
            log_path = os.path.join(self._settings.data_path, self.ERROR_REPORT_NAME)
            msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction Parameters Incomplete", msg)
            self._error_report(traceback.format_exc())
            return None
        
    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        self.scripter.update()
        
        try:
            self.scripter.apply()
            
            # Update widgets
            self.scripter.push_state()
        except RuntimeError, e:
            if self._settings.debug:
                msg = "Reduction could not be executed:\n\n%s" % unicode(traceback.format_exc())
            else:
                msg = "Reduction could not be executed:\n\n%s" % sys.exc_value
                log_path = os.path.join(self._settings.data_path, self.ERROR_REPORT_NAME)
                msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path                
            self._warning("Reduction failed", msg)
            self._error_report(traceback.format_exc())
        except:
            msg = "Reduction could not be executed:\n\n%s" % sys.exc_value
            msg += "\n\nPlease check your reduction parameters\n"
            log_path = os.path.join(self._settings.data_path, self.ERROR_REPORT_NAME)
            msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path            
            self._warning("Reduction failed", msg)
            self._error_report(traceback.format_exc())
        
    def _error_report(self, trace=''):
        """
            Try to dump the state of the UI to a file, with a traceback
            if available.
        """
        log_path = os.path.join(self._settings.data_path, self.ERROR_REPORT_NAME)
        f = open(log_path, 'w')
        reduction = self.scripter.to_xml()
        f.write("<Report>\n")
        f.write(reduction)
        f.write("<ErrorReport>")
        f.write(trace)
        f.write("</ErrorReport>")
        f.write("</Report>")
        f.close()
        
    def get_tabs(self):
        """
            Returns a list of widgets used to populate the central tab widget
            of the interface.
        """
        tab_list = []
        for item in self.widgets:
            tab_list.append([item.name, item])
        return tab_list

    def reset(self):
        """
            Reset the interface
        """
        self.scripter.reset()
        self.scripter.push_state()
        