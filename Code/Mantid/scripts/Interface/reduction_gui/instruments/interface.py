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
    LAST_REDUCTION_NAME = ".mantid_last_reduction.xml"
    ERROR_REPORT_DIR = ""

    def __init__(self, name, settings):
        """
            Initialization
            @param name: name of the instrument (string)
            @param settings:
        """
        ## List of widgets with associated observers
        self.widgets = []

        # A handle to the live data button widget (usually an instance of MWRunFiles)
        self._livebuttonwidget = None

        # Scripter object to interface with Mantid
        self.scripter = BaseReductionScripter(name=name)

        # General settings
        self._settings = settings

        # Error report directory
        self.ERROR_REPORT_DIR = os.path.expanduser('~')
        self.ERROR_REPORT_NAME = InstrumentInterface.ERROR_REPORT_DIR
        self.LAST_REDUCTION_NAME = InstrumentInterface.LAST_REDUCTION_NAME

    def attach(self, widget):
        """
            Attach a widget to the interface and hook it up to its observer/scripter.
            @param widget: QWidget object
        """
        self.widgets.append(widget)
        if widget.live_button_widget() is not None:
            self._livebuttonwidget = widget.live_button_widget()
            self._livebuttonwidget.liveButtonPressed.connect(self.live_button_toggled)
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

    def load_last_reduction(self):
        try:
            red_path = os.path.join(self.ERROR_REPORT_DIR, self.LAST_REDUCTION_NAME)
            if os.path.isfile(red_path):
                self.load_file(red_path)
            else:
                self.reset()
        except:
            print "Could not load last reduction\n  %s" % str(traceback.format_exc())
            self.reset()

    def load_file(self, file_name):
        """
            Load an XML file containing reduction parameters and
            populate the UI with them
            @param file_name: XML file to be loaded
        """
        if self.scripter.check_xml_compatibility(file_name, 'Instrument'):
            self.scripter.from_xml(file_name)
            self.scripter.push_state()
        elif self.scripter.check_xml_compatibility(file_name, 'SetupInfo'):
            self.scripter.from_xml(file_name, True)
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
                log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
                msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction Parameters Incomplete", msg)
            self._error_report(traceback.format_exc())
            return None
        except:
            msg = "The following error was encountered:\n\n%s" % sys.exc_info()[0]
            msg += "\n\nPlease check your reduction parameters\n"
            log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
            msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction Parameters Incomplete", msg)
            self._error_report(traceback.format_exc())
            return None

    def remote_resources_available(self):
        """
            Returns whether or not the application is cluster-enabled.
            The Remote algorithms have to be available and the
            cluster submission property has to be ON.
        """
        # Check whether Mantid is available
        try:
            from mantid.kernel import ConfigService
            from mantid.api import AlgorithmFactory

            if "SubmitRemoteJob" in AlgorithmFactory.getRegisteredAlgorithms(True):
                config = ConfigService.Instance()
                if config.hasProperty("cluster.submission") \
                and config.getString("cluster.submission").lower()=='on':
                    return True

            return False
        except:
            return False

    def cluster_submit(self, user, pwd, resource=None,
                       nodes=4, cores_per_node=4, job_name=None):
        """
            Pass the interface data to the scripter for parallel reduction
        """
        self.scripter.update()
        if not self.live_button_is_checked():
            try:
                # Determine where the write the script
                job_data_dir = self._settings.data_output_dir
                if job_data_dir is None:
                    job_data_dir = os.path.expanduser('~')

                self.scripter.cluster_submit(job_data_dir, user, pwd, resource, nodes, cores_per_node, job_name)
            except:
                msg = "The following error was encountered:\n\n%s" % sys.exc_value
                msg += "\n\nPlease check your reduction parameters\n"
                log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
                msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
                self._warning("Reduction Parameters Incomplete", msg)
                self._error_report(traceback.format_exc())
        else:
            self._warning("Runtime error","You cannot send live data to cluster")

    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        self.scripter.update()

        # Save the last reduction for later
        try:
            red_path = os.path.join(self.ERROR_REPORT_DIR, self.LAST_REDUCTION_NAME)
            self.save_file(red_path)
        except:
            print "Could not save last reduction\n  %s" % str(traceback.format_exc())

        try:
            self.set_running(True)
            if self.live_button_is_checked():
                # Intercept and redirect if live data requested
                self.scripter.apply_live()
            else:
                # Otherwise take the 'normal' path
                self.scripter.apply()
            self.set_running(False)
        except RuntimeError, e:
            if self._settings.debug:
                msg = "Reduction could not be executed:\n\n%s" % unicode(traceback.format_exc())
            else:
                msg = "Reduction could not be executed:\n\n%s" % sys.exc_value
                log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
                msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction failed", msg)
            self._error_report(traceback.format_exc())
        except:
            msg = "Reduction could not be executed:\n\n%s" % sys.exc_value
            msg += "\n\nPlease check your reduction parameters\n"
            log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
            msg += "\n\nWhen contacting the Mantid Team, please send this file:\n%s\n" % log_path
            self._warning("Reduction failed", msg)
            self._error_report(traceback.format_exc())
        # Update widgets
        self.scripter.push_state()

    def _error_report(self, trace=''):
        """
            Try to dump the state of the UI to a file, with a traceback
            if available.
        """
        trace = trace.replace('<', ' ')
        trace = trace.replace('>', ' ')
        log_path = os.path.join(self.ERROR_REPORT_DIR, self.ERROR_REPORT_NAME)
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

    def set_running(self, is_running=True):
        """
            Tell the widgets whether they are running or not
        """
        for widget in self.widgets:
            widget.is_running(is_running)

    def has_advanced_version(self):
        """
            Returns true if the instrument has simple and advanced views
        """
        return False

    def is_cluster_enabled(self):
        """
            Returns true if the instrument is compatible with remote submission
        """
        return False

    def is_live_enabled(self):
        """
            Returns true if the instrument interface includes a live data button
        """
        return self._livebuttonwidget is not None

    def live_button_is_checked(self):
        """
            Returns true if there is a live button and it is selected
        """
        return self.is_live_enabled() and self._livebuttonwidget.liveButtonIsChecked()

    def live_button_toggled(self,checked):
        """
            Called as a slot when the live button is pressed to make any necessary settings
            to other widgets.
            @param checked: True if the button has been checked, false if unchecked
        """
        for item in self.widgets:
            item.live_button_toggled_actions(checked)

    def reset(self):
        """
            Reset the interface
        """
        self.scripter.reset()
        self.scripter.push_state()

