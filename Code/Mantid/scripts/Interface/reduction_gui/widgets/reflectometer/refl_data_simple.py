from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import time
import sys
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets
from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_data_refl_simple

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from MantidFramework import *
    mtd.initialise(False)
    from mantidsimple import *
    import _qti
    from reduction.instruments.reflectometer import data_manipulation

    IS_IN_MANTIDPLOT = True
except:
    pass

class DataReflWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Data"      
    GeneralSettings.instrument_name = 'REF_L'
    peak_pixel_range = []
    background_pixel_range = []

    def __init__(self, parent=None, state=None, settings=None, name="REFL", data_proxy=None):      
        super(DataReflWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_data_refl_simple.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataSeries())

    def initialize_content(self):
        
        # Validators
        self._summary.data_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_from_pixel))
        self._summary.data_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_to_pixel))
        self._summary.data_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel1))
        self._summary.data_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel1))
        self._summary.data_from_tof.setValidator(QtGui.QDoubleValidator(self._summary.data_from_tof))
        self._summary.data_to_tof.setValidator(QtGui.QDoubleValidator(self._summary.data_to_tof))
        
        self._summary.x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_min_edit))
        self._summary.x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_max_edit))
        self._summary.norm_x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_min_edit))
        self._summary.norm_x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_max_edit))

        self._summary.log_scale_chk.setChecked(True)
        self._summary.q_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.q_min_edit))
        self._summary.q_step_edit.setValidator(QtGui.QDoubleValidator(self._summary.q_step_edit))
        
        self._summary.angle_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_edit))
        self._summary.angle_offset_error_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_error_edit))

        self._summary.norm_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_from_pixel))
        self._summary.norm_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_to_pixel))
        self._summary.norm_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_from_pixel1))
        self._summary.norm_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_to_pixel1))

        # Event connections
        self.connect(self._summary.data_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._data_low_res_clicked)
        self.connect(self._summary.norm_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_low_res_clicked)
        self.connect(self._summary.norm_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_clicked)
        self.connect(self._summary.norm_background_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_background_clicked)
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.tof_range_switch, QtCore.SIGNAL("clicked(bool)"), self._tof_range_clicked)
        self.connect(self._summary.plot_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y)
        self.connect(self._summary.plot_tof_btn, QtCore.SIGNAL("clicked()"), self._plot_tof)
        self.connect(self._summary.add_dataset_btn, QtCore.SIGNAL("clicked()"), self._add_data)
        self.connect(self._summary.angle_list, QtCore.SIGNAL("itemSelectionChanged()"), self._angle_changed)
        self.connect(self._summary.remove_btn, QtCore.SIGNAL("clicked()"), self._remove_item)
        
        # Set up the automated reduction options
        self._summary.auto_reduce_check.setChecked(False)
        self._auto_reduce(False)
        self.connect(self._summary.auto_reduce_check, QtCore.SIGNAL("clicked(bool)"), self._auto_reduce)
        self.connect(self._summary.auto_reduce_btn, QtCore.SIGNAL("clicked()"), self._create_auto_reduce_template)
        
        # If we do not have access to /SNS, don't display the automated reduction options
        if not os.path.isdir("/SNS/REF_L"):
            self._summary.auto_reduce_check.hide()
        
    def _create_auto_reduce_template(self):
        m = self.get_editing_state()
        m.data_files = ["runNumber"]
        reduce_script = m.to_script(True)
        
        content =  "# Script automatically generated by Mantid on %s\n" % time.ctime()
        content += "import sys\n"
        content += "import os\n"
        content += "if (os.environ.has_key(\"MANTIDPATH\")):\n"
        content += "    del os.environ[\"MANTIDPATH\"]\n"
        content += "sys.path.insert(0,'/opt/mantidnightly/bin')\n"
        content += "from MantidFramework import mtd\n"
        content += "mtd.initialize()\n"
        content += "from mantidsimple import *\n\n"
        
        content += "runNumber=sys.argv[1]\n"
        content += "outputDir=sys.argv[2]\n\n"

        content += reduce_script

        content += "\n"
        content += "file_name = 'reflectivity_'+runNumber+'.txt'\n"
        content += "file_path = os.path.join(outputDir,file_name)\n"
        content += "SaveAscii(Filename=file_path,\n"
        content += "          InputWorkspace='reflectivity_'+runNumber,\n" 
        content += "          Separator='Tab',\n"
        content += "          CommentIndicator='# ')\n"
        
        # Place holder for reduction script
        content += "\n"
        content += "# Place holder for python script\n"
        content += "file_path = os.path.join(outputDir, 'REF_L_'+runNumber+'.py')\n"
        content += "f=open(file_path,'w')\n"
        content += "f.write('# Empty file')\n"
        content += "f.close()\n\n"
        
        # Reduction option to load into Mantid
        xml_str = "<Reduction>\n"
        xml_str += "  <instrument_name>REF_L</instrument_name>\n"
        xml_str += "  <timestamp>%s</timestamp>\n" % time.ctime()
        xml_str += "  <python_version>%s</python_version>\n" % sys.version
        if IS_IN_MANTIDPLOT:
            xml_str += "  <mantid_version>%s</mantid_version>\n" % mantid_build_version()
        xml_str += m.to_xml()
        xml_str += "</Reduction>\n"
        
        content += "# Reduction options for loading into Mantid\n"
        content += "file_path = os.path.join(outputDir, 'REF_L_'+runNumber+'.xml')\n"
        content += "f=open(file_path,'w')\n"
        content += "f.write(\"\"\"%s\"\"\")\n" % xml_str
        content += "f.close()\n"

        home_dir = os.path.expanduser('~')
        f=open(os.path.join(home_dir,"reduce_REF_L.py"),'w')
        f.write(content)
        f.close()
        
        # Check whether we can write to the system folder
        def _report_error(error=None):
            message = ""
            if error is not None:
                message += error+'\n\n'
            else:    
                message += "The automated reduction script could not be saved.\n\n"
            message += "Your script has been saved in your home directory:\n"
            message += os.path.join(home_dir,"reduce_REF_L.py")
            message += "\n\nTry copying it by hand in %s\n" % sns_path
            QtGui.QMessageBox.warning(self, "Error saving automated reduction script", message)
        
        sns_path = "/SNS/REF_L/shared/autoreduce"
        if os.path.isdir(sns_path):
            if os.access(sns_path, os.W_OK):
                file_path = os.path.join(sns_path,"reduce_REF_L.py")
                if os.path.isfile(file_path) and not os.access(file_path, os.W_OK):
                    _report_error("You do not have permissions to overwrite %s." % file_path)
                    return
                try:
                    f = open(file_path,'w')
                    f.write(content)
                    f.close()
                    QtGui.QMessageBox.information(self, "Automated reduction script saved", 
                                           "The automated reduction script has been updated")
                except:
                    _report_error()
            else:
                _report_error("You do not have permissions to write to %s." % sns_path)
        else:
            _report_error("The autoreduce directory doesn't exist.\n"
                          "Your instrument may not be set up for automated reduction.")
        
    def _auto_reduce(self, is_checked=False):
        if is_checked:
            
            self._summary.auto_reduce_help_label.show()
            self._summary.auto_reduce_tip_label.show()
            self._summary.auto_reduce_btn.show()
        else:
            self._summary.auto_reduce_help_label.hide()
            self._summary.auto_reduce_tip_label.hide()
            self._summary.auto_reduce_btn.hide()
    
    def _remove_item(self):
        row = self._summary.angle_list.currentRow()
        if row>=0:
            self._summary.angle_list.takeItem(row)

    def is_running(self, is_running):
        """
            Enable/disable controls depending on whether a reduction is running or not
            @param is_running: True if a reduction is running
        """
        super(DataReflWidget, self).is_running(is_running)
        self._summary.plot_count_vs_y_btn.setEnabled(not is_running)
        self._summary.plot_tof_btn.setEnabled(not is_running)

    def _data_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.data_background_from_pixel1.setEnabled(is_checked)
        self._summary.data_background_from_pixel1_label.setEnabled(is_checked)
        self._summary.data_background_to_pixel1.setEnabled(is_checked)
        self._summary.data_background_to_pixel1_label.setEnabled(is_checked)
        
    def _norm_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.norm_background_from_pixel1.setEnabled(is_checked)
        self._summary.norm_background_from_pixel1_label.setEnabled(is_checked)
        self._summary.norm_background_to_pixel1.setEnabled(is_checked)
        self._summary.norm_background_to_pixel1_label.setEnabled(is_checked)
        
    def _data_low_res_clicked(self, is_checked):
        """
            This is reached when the user clicks the Data Low-Res axis range switch
        """
        self._summary.data_low_res_from_label.setEnabled(is_checked)
        self._summary.x_min_edit.setEnabled(is_checked)
        self._summary.data_low_res_to_label.setEnabled(is_checked)
        self._summary.x_max_edit.setEnabled(is_checked)
            
    def _norm_low_res_clicked(self, is_checked):
        """
            This is reached when the user clicks the Data Low-Res axis range switch
        """
        self._summary.norm_low_res_from_label.setEnabled(is_checked)
        self._summary.norm_x_min_edit.setEnabled(is_checked)
        self._summary.norm_low_res_to_label.setEnabled(is_checked)
        self._summary.norm_x_max_edit.setEnabled(is_checked)

    def _norm_clicked(self, is_checked):
        """
            This is reached when the user clicks the Normalization switch and will
            turn on/off all the option related to the normalization file
        """
        self._summary.norm_run_number_label.setEnabled(is_checked)
        self._summary.norm_run_number_edit.setEnabled(is_checked)
        self._summary.norm_peak_selection_label.setEnabled(is_checked)
        self._summary.norm_peak_selection_from_label.setEnabled(is_checked)
        self._summary.norm_peak_from_pixel.setEnabled(is_checked)
        self._summary.norm_peak_selection_to_label.setEnabled(is_checked)
        self._summary.norm_peak_to_pixel.setEnabled(is_checked)
        
        self._summary.norm_background_switch.setEnabled(is_checked)
        if (not(is_checked)):
            self._norm_background_clicked(False)
        else:
            NormBackFlag = self._summary.norm_background_switch.isChecked()
            self._norm_background_clicked(NormBackFlag)
        
        self._summary.norm_low_res_range_switch.setEnabled(is_checked)
        if (not(is_checked)):
            self._norm_low_res_clicked(False)
        else:
            LowResFlag = self._summary.norm_low_res_range_switch.isChecked()
            self._norm_low_res_clicked(LowResFlag)

    def _tof_range_clicked(self, is_checked):
        """
            This is reached by the TOF range switch
        """
        self._summary.tof_min_label.setEnabled(is_checked)
        self._summary.data_from_tof.setEnabled(is_checked)
        self._summary.tof_min_label2.setEnabled(is_checked)
        self._summary.tof_max_label.setEnabled(is_checked)
        self._summary.data_to_tof.setEnabled(is_checked)
        self._summary.tof_max_label2.setEnabled(is_checked)
        self._summary.plot_tof_btn.setEnabled(is_checked)

    def _plot_count_vs_y(self):
        if not IS_IN_MANTIDPLOT:
            return
        
        try:
            f = FileFinder.findRuns(str(self._summary.data_run_number_edit.text()))
        except:
            f = FileFinder.findRuns("REF_L%s" % str(self._summary.data_run_number_edit.text()))

        if len(f)>0 and os.path.isfile(f[0]):
            def call_back(xmin, xmax):
                self._summary.data_peak_from_pixel.setText("%-d" % int(xmin))
                self._summary.data_peak_to_pixel.setText("%-d" % int(xmax))
            data_manipulation.counts_vs_y_distribution(f[0], call_back)
                
    def _plot_tof(self):
        if not IS_IN_MANTIDPLOT:
            return
        
        try:
            f = FileFinder.findRuns(str(self._summary.norm_run_number_edit.text()))
        except:
            f = FileFinder.findRuns("REF_L%s" % str(self._summary.norm_run_number_edit.text()))
            
        if len(f)>0 and os.path.isfile(f[0]):
            def call_back(xmin, xmax):
                self._summary.data_from_tof.setText("%-d" % int(xmin))
                self._summary.data_to_tof.setText("%-d" % int(xmax))
            data_manipulation.tof_distribution(f[0], call_back)

    def _add_data(self):
        state = self.get_editing_state()
        # Check whether it's already in the list
        run_numbers = self._summary.data_run_number_edit.text()
        list_items = self._summary.angle_list.findItems(run_numbers, QtCore.Qt.MatchFixedString)
        if len(list_items)>0:
            list_items[0].setData(QtCore.Qt.UserRole, state)
        else:
            item_widget = QtGui.QListWidgetItem(run_numbers, self._summary.angle_list)
            item_widget.setData(QtCore.Qt.UserRole, state)

    def _angle_changed(self):
        current_item =  self._summary.angle_list.currentItem()
        if current_item is not None:
            state = current_item.data(QtCore.Qt.UserRole).toPyObject()
            self.set_editing_state(state)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object    
        """
        if False and IS_IN_MANTIDPLOT:
            ws_name = "reflectivity"
            ws_list = [n for n in mtd.keys() if n.startswith(ws_name)]
            g = _qti.app.graph(ws_name)
            if g is None and len(ws_list)>0:
                g = _qti.app.mantidUI.pyPlotSpectraList(ws_list,[0],True)
                g.setName(ws_name)  
        
        self._summary.angle_list.clear()
        if len(state.data_sets)==1 and state.data_sets[0].data_files[0]==0:
            pass
        else: 
            for item in state.data_sets:
                if item is not None:
                    item_widget = QtGui.QListWidgetItem(unicode(str(','.join([str(i) for i in item.data_files]))), self._summary.angle_list)
                    item_widget.setData(QtCore.Qt.UserRole, item)

        if len(state.data_sets)>0:
            self.set_editing_state(state.data_sets[0])
            self._summary.angle_list.setCurrentRow(0)
            
            # Common Q binning
            self._summary.q_min_edit.setText(str(state.data_sets[0].q_min))
            self._summary.log_scale_chk.setChecked(state.data_sets[0].q_step<0)
            self._summary.q_step_edit.setText(str(math.fabs(state.data_sets[0].q_step)))
            
            # Common angle offset
            self._summary.angle_offset_edit.setText(str(state.data_sets[0].angle_offset))
            self._summary.angle_offset_error_edit.setText(str(state.data_sets[0].angle_offset_error))

    def set_editing_state(self, state):

        #Peak from/to pixels
        self._summary.data_peak_from_pixel.setText(str(state.DataPeakPixels[0]))
        self._summary.data_peak_to_pixel.setText(str(state.DataPeakPixels[1]))
        
        self._summary.x_min_edit.setText(str(state.x_range[0]))
        self._summary.x_max_edit.setText(str(state.x_range[1]))
        
        self._summary.norm_x_min_edit.setText(str(state.norm_x_min))
        self._summary.norm_x_max_edit.setText(str(state.norm_x_max))
        
        #Background flag
        self._summary.data_background_switch.setChecked(state.DataBackgroundFlag)
        self._data_background_clicked(state.DataBackgroundFlag)

        #Background from/to pixels
        self._summary.data_background_from_pixel1.setText(str(state.DataBackgroundRoi[0]))
        self._summary.data_background_to_pixel1.setText(str(state.DataBackgroundRoi[1]))
        
        #from TOF and to TOF
        self._summary.data_from_tof.setText(str(state.DataTofRange[0]))
        self._summary.data_to_tof.setText(str(state.DataTofRange[1]))
        
        self._summary.data_run_number_edit.setText(str(','.join([str(i) for i in state.data_files])))
        
        #normalization flag
        self._summary.norm_switch.setChecked(state.NormFlag)
        
        # Normalization options
        self._summary.norm_run_number_edit.setText(str(state.norm_file))
        self._summary.norm_peak_from_pixel.setText(str(state.NormPeakPixels[0]))
        self._summary.norm_peak_to_pixel.setText(str(state.NormPeakPixels[1]))
        
        self._summary.norm_background_switch.setChecked(state.NormBackgroundFlag)
        self._norm_background_clicked(state.NormBackgroundFlag)

        self._summary.norm_background_from_pixel1.setText(str(state.NormBackgroundRoi[0]))
        self._summary.norm_background_to_pixel1.setText(str(state.NormBackgroundRoi[1]))
        
        # Q binning
        #self._summary.q_min_edit.setText(str(state.q_min))
        #self._summary.log_scale_chk.setChecked(state.q_step<0)
        #self._summary.q_step_edit.setText(str(math.fabs(state.q_step)))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = self.get_editing_state()
        state = DataSeries()
        state_list = []
        
        # Common Q binning
        q_min = float(self._summary.q_min_edit.text())
        q_step = float(self._summary.q_step_edit.text())
        if self._summary.log_scale_chk.isChecked():
            q_step = -q_step
        
        # Angle offset
        angle_offset = float(self._summary.angle_offset_edit.text())
        angle_offset_error = float(self._summary.angle_offset_error_edit.text())
                
        for i in range(self._summary.angle_list.count()):
            data = self._summary.angle_list.item(i).data(QtCore.Qt.UserRole).toPyObject()
            # Over-write Q binning with common binning
            data.q_min = q_min
            data.q_step = q_step
        
            # Over-write angle offset
            data.angle_offset = angle_offset
            data.angle_offset_error = angle_offset_error

            ##
            # Add here states that are relevant to the interface (all the runs)
            ##
            
            state_list.append(data)
        state.data_sets = state_list
        
        return state
    
    def get_editing_state(self):
        m = DataSets()
        
        #Peak from/to pixels
        m.DataPeakPixels = [int(self._summary.data_peak_from_pixel.text()),
                            int(self._summary.data_peak_to_pixel.text())] 
        
        m.x_range = [int(self._summary.x_min_edit.text()),
                     int(self._summary.x_max_edit.text())]
        
        m.norm_x_min = int(self._summary.norm_x_min_edit.text())
        m.norm_x_max = int(self._summary.norm_x_max_edit.text())
        
        #Background flag
        m.DataBackgroundFlag = self._summary.data_background_switch.isChecked()

        #Background from/to pixels
        roi1_from = int(self._summary.data_background_from_pixel1.text())
        roi1_to = int(self._summary.data_background_to_pixel1.text())
        m.DataBackgroundRoi = [roi1_from, roi1_to, 0, 0]

        #from TOF and to TOF
        from_tof = float(self._summary.data_from_tof.text())
        to_tof = float(self._summary.data_to_tof.text())
        m.DataTofRange = [from_tof, to_tof]
        
        datafiles = str(self._summary.data_run_number_edit.text()).split(',')
        m.data_files = [int(i) for i in datafiles]
    
        # Normalization flag
        m.NormFlag = self._summary.norm_switch.isChecked()

        # Normalization options
        m.norm_file = int(self._summary.norm_run_number_edit.text())
        m.NormPeakPixels = [int(self._summary.norm_peak_from_pixel.text()),
                            int(self._summary.norm_peak_to_pixel.text())]   
        
        #Background flag
        m.NormBackgroundFlag = self._summary.norm_background_switch.isChecked()

        #Background from/to pixels
        roi1_from = int(self._summary.norm_background_from_pixel1.text())
        roi1_to = int(self._summary.norm_background_to_pixel1.text())
        m.NormBackgroundRoi = [roi1_from, roi1_to]
        
        #m.q_min = float(self._summary.q_min_edit.text())
        #m.q_step = float(self._summary.q_step_edit.text())
        #if self._summary.log_scale_chk.isChecked():
        #    m.q_step = -m.q_step

        ##
        # Add here states that are data file dependent
        ##
          
        return m