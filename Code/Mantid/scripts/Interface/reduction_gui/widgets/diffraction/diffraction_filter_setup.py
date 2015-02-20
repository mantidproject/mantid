#pylint: disable=invalid-name
################################################################################
# Event Filtering (and advanced) Setup Widget
################################################################################
from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util

from reduction_gui.reduction.diffraction.diffraction_filter_setup_script import FilterSetupScript
import ui.diffraction.ui_diffraction_filter_setup
import ui.diffraction.ui_filter_info

import os

IS_IN_MANTIDPLOT = False
try:
    import mantid.simpleapi as api
    IS_IN_MANTIDPLOT = True
except:
    pass

class FilterSetupWidget(BaseWidget):
    """ Widget that presents event filters setup
    """
    # Widge name
    name = "Event Filters Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """ Initialization
        """
        super(FilterSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        #print "[FilterSetupWidget.Init]: settings is of type %s.  data type is of type %s.
        #       DBx237. " % (type(settings), type(data_type))

        class FilterSetFrame(QtGui.QFrame, ui.diffraction.ui_diffraction_filter_setup.Ui_Frame):
            """ Define class linked to UI Frame
            """
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = FilterSetFrame(self)
        self._layout.addWidget(self._content)
        self._instrument_name = settings.instrument_name
        self._facility_name = settings.facility_name
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(FilterSetupScript(self._instrument_name))

        return

    def initialize_content(self):
        """ Initialize content/UI
        """
        # Initial value
        #   combo-boxes
        self._content.timeunit_combo.setCurrentIndex(0)
        self._content.valuechange_combo.setCurrentIndex(0)
        self._content.logbound_combo.setCurrentIndex(0)

        self._content.log_name_combo.clear()

        #   check-boxes
        self._content.timefilter_checkBox.setChecked(False)
        self._content.logvaluefilter_checkBox.setChecked(False)

        #   disable some input
        self._content.logname_edit.setEnabled(False)
        self._content.logminvalue_edit.setEnabled(False)
        self._content.logmaxvalue_edit.setEnabled(False)
        self._content.logintervalvalue_edit.setEnabled(False)
        self._content.valuechange_combo.setEnabled(False)
        self._content.logtol_edit.setEnabled(False)
        self._content.timetol_edit.setEnabled(False)
        self._content.logbound_combo.setEnabled(False)

        boolvalue = False
        self._content.timintervallength_edit.setEnabled(boolvalue)
        self._content.timeunit_combo.setEnabled(boolvalue)

        #   radio buttons
        # self._content.usesize_radiob.setChecked(True)

        # Constraints/Validator
        #   integers
        # iv0 = QtGui.QIntValidator(self._content.numtimeinterval_edit)
        # iv0.setBottom(0)
        # self._content.numtimeinterval_edit.setValidator(iv0)

        iv1 = QtGui.QIntValidator(self._content.run_number_edit)
        iv1.setBottom(0)
        self._content.run_number_edit.setValidator(iv1)

        #   floats
        dv0 = QtGui.QDoubleValidator(self._content.starttime_edit)
        dv0.setBottom(0.)
        self._content.starttime_edit.setValidator(dv0)

        dv1 = QtGui.QDoubleValidator(self._content.stoptime_edit)
        dv1.setBottom(0.)
        self._content.stoptime_edit.setValidator(dv1)

        dv2 = QtGui.QDoubleValidator(self._content.timintervallength_edit)
        dv2.setBottom(0.)
        self._content.timintervallength_edit.setValidator(dv2)

        # Default states

        # Connections from action/event to function to handle
        self.connect(self._content.timefilter_checkBox, QtCore.SIGNAL("stateChanged(int)"),\
                self._filterbytime_statechanged)

        self.connect(self._content.logvaluefilter_checkBox, QtCore.SIGNAL("stateChanged(int)"),\
                self._filterbylogvalue_statechanged)

        self.connect(self._content.load_button, QtCore.SIGNAL("clicked()"),\
                self._run_number_changed)

        # self.connect(self._content.run_number_edit, QtCore.SIGNAL("textChanged(QString)"), self._run_number_changed)
        self.connect(self._content.run_number_edit, QtCore.SIGNAL("returnPressed()"), self._run_number_changed)

        self.connect(self._content.plot_log_button, QtCore.SIGNAL("clicked()"),\
                self._plot_log_clicked)

        self.connect(self._content.syn_logname_button, QtCore.SIGNAL("clicked()"),\
                self._sync_logname_clicked)

        self.connect(self._content.help_button, QtCore.SIGNAL("clicked()"),\
                self._show_help)

        # Validated widgets
        # self._connect_validated_lineedit(self._content.sample_edit)

        return

    def set_state(self, state):
        """ Populate the UI elements with the data from the given state, i.e., a coupled FilterSetupScript.
            @param state: FilterSetupScript object
        """
        # Title
        self._content.title_edit.setText(str(state.titleofsplitters))

        # General
        if state.starttime is not None and state.starttime != "":
            self._content.starttime_edit.setText(str(state.starttime))
        if state.stoptime is not None and state.stoptime != "":
            self._content.stoptime_edit.setText(str(state.stoptime))

        # Chop in time
        self._content.timefilter_checkBox.setChecked(state.filterbytime)
        # if state.numbertimeinterval is not None and state.numbertimeinterval != "":
        #     self._content.numtimeinterval_edit.setText(str(state.numbertimeinterval))
        if state.lengthtimeinterval is not None and state.lengthtimeinterval != "":
            self._content.timintervallength_edit.setText(str(state.lengthtimeinterval))
        if state.unitoftime != "":
            index = self._content.timeunit_combo.findText(str(state.unitoftime))
            if index >= 0:
                self._content.timeunit_combo.setCurrentIndex(index)
            else:
                self._content.timeunit_combo.setCurrentIndex(0)
                print "Input value of unit of time '%s' is not allowed. " % (state.unitoftime)
        else:
            # Default
            self._content.timeunit_combo.setCurrentIndex(0)

        # Chop by log value
        self._content.logvaluefilter_checkBox.setChecked(state.filterbylogvalue)
        self._content.logname_edit.setText(state.logname)
        if state.minimumlogvalue is not None:
            self._content.logminvalue_edit.setText(str(state.minimumlogvalue))
        else:
            self._content.logminvalue_edit.setText("")
        if state.maximumlogvalue is not None:
            self._content.logmaxvalue_edit.setText(str(state.maximumlogvalue))
        else:
            self._content.logmaxvalue_edit.setText("")
        if state.logvalueinterval is not None:
            self._content.logintervalvalue_edit.setText(str(state.logvalueinterval))
        else:
            self._content.logintervalvalue_edit.setText("")
        if state.logvaluetolerance is not None:
            self._content.logtol_edit.setText(str(state.logvaluetolerance))
        if state.filterlogvaluebychangingdirection != "":
            index = self._content.valuechange_combo.findText(str(state.filterlogvaluebychangingdirection))
            if index >= 0:
                self._content.valuechange_combo.setCurrentIndex(index)
            else:
                self._content.valuechange_combo.setCurrentIndex(0)
                print "Input value of filter log value by changing direction '%s' is not allowed." % (state.filterlogvaluebychangingdirection)
        else:
            # Default
            self._content.valuechange_combo.setCurrentIndex(0)

        if state.logboundary != "":
            index = self._content.logbound_combo.findText(str(state.logboundary))
            if index >= 0:
                self._content.logbound_combo.setCurrentIndex(index)
            else:
                self._content.logbound_combo.setCurrentIndex(0)
                print "Input value for log boundary '%s' is not allowed." % (state.logboundary)
        else:
            # Default
            self._content.logbound_combo.setCurrentIndex(0)

        if state.timetolerance is not None:
            self._content.timetol_edit.setText(str(state.timetolerance))

        """ FIXME - Impliement later...
        self._content.timintervallength_edit.setText(str(state.timeinterval))
        """

        return


    def get_state(self):
        """ Returns a FilterSetupScript with the state of Filter_Setup_Interface
        Set up all the class parameters in FilterSetupScrpt with values in the content
        """
        s = FilterSetupScript(self._instrument_name)

        # Title
        s.titleofsplitters    = self._content.title_edit.text()

        # General
        s.starttime           = self._content.starttime_edit.text()
        s.stoptime            = self._content.stoptime_edit.text()

        # Filter by time
        s.filterbytime        = self._content.timefilter_checkBox.isChecked()
        # s.numbertimeinterval  = self._content.numtimeinterval_edit.text()
        s.lengthtimeinterval  = self._content.timintervallength_edit.text()
        s.unitoftime          = self._content.timeunit_combo.currentText()

        # Filter by log value
        s.filterbylogvalue    = self._content.logvaluefilter_checkBox.isChecked()
        s.logname             = self._content.logname_edit.text()
        s.minimumlogvalue     = self._content.logminvalue_edit.text()
        s.maximumlogvalue     = self._content.logmaxvalue_edit.text()
        s.logvalueinterval    = self._content.logintervalvalue_edit.text()
        # if self._content.usesize_radiob.isChecked():
        #     s.numlogvalueinterval = ""
        #     s.logvalueinterval    = self._content.logintervalvalue_edit.text()
        # elif self._content.usenumstep_radiob.isChecked():
        #     s.numlogvalueinterval = self._content.numloginterval_edit.text()
        #     s.logvalueinterval    = ""
        s.timetolerance       = self._content.timetol_edit.text()
        s.logboundary         = self._content.logbound_combo.currentText()
        s.logvaluetolerance   = self._content.logtol_edit.text()
        # s.logvaluetimesections = self._content.logvaluesection_edit.text()
        s.filterlogvaluebychangingdirection =  self._content.valuechange_combo.currentText()

        return s

    def _run_number_changed(self):
        """ Handling event if run number is changed... If it is a valid run number,
        the load the meta data
        """
        from datetime import datetime

        # 1. Form the file
        newrunnumberstr = self._content.run_number_edit.text()
        instrument = self._instrument_name
        eventnxsname = "%s_%s_event.nxs" % (instrument, newrunnumberstr)
        msg = str("Load event nexus file %s" % (eventnxsname))
        self._content.info_text_browser.setText(msg)

        # 2. Load file
        metawsname = "%s_%s_meta" % (instrument, newrunnumberstr)
        try:
            metaws = api.Load(Filename=str(eventnxsname), OutputWorkspace=str(metawsname), MetaDataOnly=True)
        except ValueError:
            metaws = None

        # 3. Update the log name combo box
        if metaws is None:
            self._content.info_text_browser.setText(
                    str("Error! Failed to load data file %s.  Current working directory is %s. " % (eventnxsname, os.getcwd())))
        else:
            self._metaws = metaws

            # a) Clear
            self._content.log_name_combo.clear()

            # b) Get properties
            wsrun = metaws.getRun()
            ps = wsrun.getProperties()
            properties = []
            for p in ps:
                if p.__class__.__name__ == "FloatTimeSeriesProperty":
                    if p.size() > 1:
                        properties.append(p.name)
                        print p.name, p.size()
            # ENDFOR p
            properties = sorted(properties)

            # c) Add
            for p in properties:
                self._content.log_name_combo.addItem(p)
        # ENDIFELSE

        """
        now = datetime.now()
        print "New Run Number = %s.  @ %s" % (newrunnumberstr, str(now))
        if newrunnumberstr == "":
            self._content.log_name_combo.clear()
        else:
            self._content.log_name_combo.addItem(newrunnumberstr)
        """

        return

    def _plot_log_clicked(self):
        """ Handling event if plog-log button is clicked.
        The log selected will be plotted in MantidPlot window
        """
        logname = self._content.log_name_combo.currentText()

        msg1 = str("Log %s is selected to be plotted.\n" % (logname))
        self._content.info_text_browser.setText(msg1)

        # Check with exception
        if self._metaws is None:
            msg2 = str("Error! Workspace does not exist!  Unable to plot log %s.\n" % (logname))
            self._content.info_text_browser.setText(str(msg1 +  msg2))
            return

        # Get property
        run = self._metaws.getRun()
        try:
            logproperty = run.getProperty(str(logname))
        except RuntimeError:
            # Unable to plot
            msg3 = str("Error! Workspace %s does not contain log %s. " % (str(self._metaws),\
                logname))
            self._content.info_text_browser.setText(str(msg1+msg3))
            return

        # Construct workspace
        output = api.ExportTimeSeriesLog(InputWorkspace = str(self._metaws),\
                OutputWorkspace = str(logname),\
                LogName = str(logname),\
                IsEventWorkspace = False)
        #api.DeleteWorkspace(Workspace="PercentStat")

        try:
            logws = output
        except IndexError:
            msg4 = str("Error! Workspace %s is unable to convert log %s to workspace. " % (str(self._metaws), str(logname)))
            self._content.info_text_browser.setText(str(msg1+msg4))
            return

        # Plot!
        self.plot(logws, self._content.starttime_edit, self._content.stoptime_edit)

        return

    def _filterbytime_statechanged(self):
        """ Handling event if Filter-by-time is selected
        """
        if self._content.timefilter_checkBox.isChecked():
            self._content.logvaluefilter_checkBox.setChecked(False)

            boolvalue = False
            self._content.logname_edit.setEnabled(boolvalue)
            self._content.logminvalue_edit.setEnabled(boolvalue)
            self._content.logmaxvalue_edit.setEnabled(boolvalue)
            self._content.logintervalvalue_edit.setEnabled(boolvalue)
            self._content.valuechange_combo.setEnabled(boolvalue)
            self._content.logtol_edit.setEnabled(boolvalue)
            self._content.timetol_edit.setEnabled(boolvalue)
            self._content.logbound_combo.setEnabled(boolvalue)

            boolvalue = True
            self._content.timintervallength_edit.setEnabled(boolvalue)
            self._content.timeunit_combo.setEnabled(boolvalue)
        else:
            boolvalue = False
            self._content.timintervallength_edit.setEnabled(boolvalue)
            self._content.timeunit_combo.setEnabled(boolvalue)

        return

    def _filterbylogvalue_statechanged(self):
        """ Handling event if Filter-by-logvalue is selected
        """
        if self._content.logvaluefilter_checkBox.isChecked():
            self._content.timefilter_checkBox.setChecked(False)

            boolvalue = True
            self._content.logname_edit.setEnabled(boolvalue)
            self._content.logminvalue_edit.setEnabled(boolvalue)
            self._content.logmaxvalue_edit.setEnabled(boolvalue)
            self._content.logintervalvalue_edit.setEnabled(boolvalue)
            self._content.valuechange_combo.setEnabled(boolvalue)
            self._content.logtol_edit.setEnabled(boolvalue)
            self._content.timetol_edit.setEnabled(boolvalue)
            self._content.logbound_combo.setEnabled(boolvalue)

            boolvalue = False
            self._content.timintervallength_edit.setEnabled(boolvalue)
            self._content.timeunit_combo.setEnabled(boolvalue)
        else:
            boolvalue = False
            self._content.logname_edit.setEnabled(boolvalue)
            self._content.logminvalue_edit.setEnabled(boolvalue)
            self._content.logmaxvalue_edit.setEnabled(boolvalue)
            self._content.logintervalvalue_edit.setEnabled(boolvalue)
            self._content.valuechange_combo.setEnabled(boolvalue)
            self._content.logtol_edit.setEnabled(boolvalue)
            self._content.timetol_edit.setEnabled(boolvalue)
            self._content.logbound_combo.setEnabled(boolvalue)

        return

    def _sync_logname_clicked(self):
        """ Synchronize the log name from FilterAssistant to FilterByLog
        """
        logname = self._content.log_name_combo.currentText()
        if logname is None or logname == "":
            # Error situlation
            self._content.info_text_browser.setText("No log name is selected at this moment.")
        else:
            # Sync
            self._content.logname_edit.setText(logname)

        return

    def plot(self, ws, minwidget, maxwidget):
        """ Plot
        """

        # Local definition of call back
        def call_back(xmin, xmax):
            minwidget.setText("%-f" % float(xmin))
            maxwidget.setText("%-f" % float(xmax))
            return

        self.logvalue_vs_time_distribution(workspace=ws,\
                callback=call_back)

        return

    def logvalue_vs_time_distribution(self, workspace, callback):
        """ Plot
        """
        xmin = workspace.dataX(0)[0]
        xmax = workspace.dataX(0)[-1]
        if callback is not None:
            from LargeScaleStructures import data_stitching
            data_stitching.RangeSelector.connect([workspace], callback,\
                                             xmin=xmin, xmax=xmax)

        return

    def _show_help(self):
        class HelpDialog(QtGui.QDialog, ui.diffraction.ui_filter_info.Ui_Dialog):
            def __init__(self, parent=None):
                QtGui.QDialog.__init__(self, parent)
                self.setupUi(self)
        dialog = HelpDialog(self)
        dialog.exec_()

        return

#ENDCLASSDEF
