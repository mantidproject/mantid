from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import os
from reduction_gui.reduction.sans.eqsans_sample_script import SampleData
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_trans_direct_beam
import ui.sans.ui_eqsans_sample_data
from reduction_gui.reduction.mantid_util import DataFileProxy

from hfir_sample_data import DirectBeam

class EQSANSBeamHole(BaseWidget):
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(EQSANSBeamHole, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy)
        
        if state is None:
            self.set_state(SampleData.BeamHole())      
           
    def get_state(self):
        m = SampleData.BeamHole()
        return m
            
class SampleDataWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    _method_box = None

    ## Widget name
    name = "Sample"      
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(SampleDataWidget, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy) 

        class DataFrame(QtGui.QFrame, ui.sans.ui_eqsans_sample_data.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DataFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(SampleData())
            
        self._last_direct_state = None
        self._last_spreader_state = None

    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.transmission_edit.setValidator(QtGui.QDoubleValidator(self._content.transmission_edit))
        self._content.dtransmission_edit.setValidator(QtGui.QDoubleValidator(self._content.dtransmission_edit))
        
        # Connections
        self.connect(self._content.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._data_file_browse)
        self.connect(self._content.calculate_chk, QtCore.SIGNAL("clicked(bool)"), self._calculate_clicked)
        
        self.connect(self._content.empty_button, QtCore.SIGNAL("clicked()"), self._empty_browse)
        self.connect(self._content.sample_button, QtCore.SIGNAL("clicked()"), self._sample_browse)
        
        self.connect(self._content.data_file_plot_button, QtCore.SIGNAL("clicked()"), self._data_file_plot)
        self.connect(self._content.empty_plot_button, QtCore.SIGNAL("clicked()"), self._empty_plot)
        self.connect(self._content.sample_plot_button, QtCore.SIGNAL("clicked()"), self._sample_plot)

        if not self._in_mantidplot:
            self._content.data_file_plot_button.hide()
            self._content.empty_plot_button.hide()
            self._content.sample_plot_button.hide()

    def _empty_plot(self):
        self.show_instrument(file_name=self._content.empty_edit.text)

    def _sample_plot(self):
        self.show_instrument(file_name=self._content.sample_edit.text)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.transmission_edit.setText(QtCore.QString("%6.4f" % state.transmission))
        self._content.dtransmission_edit.setText(QtCore.QString("%6.4f" % state.transmission_spread))
        
        self._content.beam_radius_edit.setText(QtCore.QString(str(state.calculation_method.beam_radius)))
        self._content.sample_edit.setText(QtCore.QString(state.calculation_method.sample_file))
        self._content.empty_edit.setText(QtCore.QString(state.calculation_method.direct_beam))

        self._content.calculate_chk.setChecked(state.calculate_transmission)
        self._content.theta_dep_chk.setChecked(state.theta_dependent)
        self._content.fit_together_check.setChecked(state.combine_transmission_frames)
        self._calculate_clicked(state.calculate_transmission)
        
        # Data file
        #   Check whether we are updating the data file
        data_files = self._get_data_files()
        current_file = ''
        if len(data_files)>0:
            current_file = data_files[0].strip()
        
        self._content.data_file_edit.setText(QtCore.QString(';'.join(state.data_files)))
        if len(state.data_files)>0:
            self._settings.last_file = state.data_files[0]
            self._settings.last_data_ws = ''

            # Store the location of the loaded file
            if len(state.data_files[0])>0:
                (folder, file_name) = os.path.split(state.data_files[0])
                self._settings.data_path = folder
                if current_file != state.data_files[0].strip():
                    self.get_data_info()
                else:
                    self._emit_experiment_parameters()


    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = SampleData()

        m.transmission = util._check_and_get_float_line_edit(self._content.transmission_edit)
        m.transmission_spread = util._check_and_get_float_line_edit(self._content.dtransmission_edit)
        
        m.calculate_transmission = self._content.calculate_chk.isChecked()
        m.theta_dependent = self._content.theta_dep_chk.isChecked()
        m.combine_transmission_frames = self._content.fit_together_check.isChecked()
        m.use_sample_dark = True
        
        d = SampleData.DirectBeam()
        d.beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit)
        d.sample_file = unicode(self._content.sample_edit.text())
        d.direct_beam = unicode(self._content.empty_edit.text())

        m.calculation_method=d
        
        # Data file
        m.data_files = self._get_data_files()

        return m

    def _data_file_browse(self):
        #   Check whether we are updating the data file
        data_files = self._get_data_files()
        current_file = ''
        if len(data_files)>0:
            current_file = data_files[0].strip()

        fname = self.data_browse_dialog(multi=True)
        if fname and len(fname)>0:
            self._content.data_file_edit.setText(';'.join(fname))   
            self._settings.last_file = fname[0] 
            self._settings.last_data_ws = ''
            if current_file != str(fname[0]).strip():
                self.get_data_info()

    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)      
        
    def _empty_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.empty_edit.setText(fname)      

    def _data_file_plot(self):
        data_files = self._get_data_files()
        if len(data_files)>0:
            self.show_instrument(data_files[0])

    def _calculate_clicked(self, is_checked):
        self._content.transmission_edit.setEnabled(not is_checked)
        self._content.dtransmission_edit.setEnabled(not is_checked)

        self._content.fit_together_check.setEnabled(is_checked)
        
        self._content.sample_label.setEnabled(is_checked)
        self._content.sample_edit.setEnabled(is_checked)
        self._content.sample_button.setEnabled(is_checked)
        self._content.sample_plot_button.setEnabled(is_checked)        

        self._content.empty_label.setEnabled(is_checked)
        self._content.empty_edit.setEnabled(is_checked)
        self._content.empty_button.setEnabled(is_checked)
        self._content.empty_plot_button.setEnabled(is_checked)        

        self._content.beam_radius_label.setEnabled(is_checked)
        self._content.beam_radius_edit.setEnabled(is_checked)

    def _get_data_files(self):
        """
            Utility method to return the list of data files in the sample data file edit box.
        """
        flist_str = str(self._content.data_file_edit.text())
        flist_str = flist_str.replace(',', ';')
        return flist_str.split(';')

    def _emit_experiment_parameters(self):
        pass
    
    def get_data_info(self):
        """
            Retrieve information from the data file and update the display
        """
        if self._data_proxy is None:
            return
        
        data_files = self._get_data_files()
        if len(data_files)<1:
            return
        fname = data_files[0]
        if len(str(fname).strip())>0:
            dataproxy = self._data_proxy(fname)
            if len(dataproxy.errors)>0:
                #QtGui.QMessageBox.warning(self, "Error", dataproxy.errors[0])
                return
            
            self._settings.last_data_ws = dataproxy.data_ws
            if dataproxy.sample_detector_distance is not None:
                self._settings.emit_key_value("sample_detector_distance", QtCore.QString(str(dataproxy.sample_detector_distance)))
            if dataproxy.sample_thickness is not None:
                self._settings.emit_key_value("sample_thickness", QtCore.QString(str(dataproxy.sample_thickness)))
            if dataproxy.beam_diameter is not None:
                self._settings.emit_key_value("beam_diameter", QtCore.QString(str(dataproxy.beam_diameter)))
             
            self._emit_experiment_parameters()    