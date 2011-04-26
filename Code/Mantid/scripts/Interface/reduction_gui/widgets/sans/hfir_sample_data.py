from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import os
from reduction_gui.reduction.sans.hfir_sample_script import SampleData
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_trans_direct_beam
import ui.sans.ui_trans_spreader
import ui.sans.ui_hfir_sample_data
from reduction_gui.reduction.mantid_util import DataFileProxy
import functools

class DirectBeam(BaseWidget):
    """
        Widget for the direct beam transmission calculation options.
    """
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(DirectBeam, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy)
        
        class DirectBeamFrame(QtGui.QGroupBox, ui.sans.ui_trans_direct_beam.Ui_GroupBox): 
            def __init__(self, parent=None):
                QtGui.QGroupBox.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DirectBeamFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(SampleData.DirectBeam())      
  
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        
        # Connections
        self.connect(self._content.sample_browse, QtCore.SIGNAL("clicked()"), self._sample_browse)
        self.connect(self._content.direct_browse, QtCore.SIGNAL("clicked()"), self._direct_browse)
        
        self.connect(self._content.sample_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.sample_edit.text))
        self.connect(self._content.direct_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.direct_edit.text))
        
        if not self._in_mantidplot:
            self._content.sample_plot.hide()
            self._content.direct_plot.hide()

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.beam_radius_edit.setText(QtCore.QString(str(state.beam_radius)))
        self._content.sample_edit.setText(QtCore.QString(state.sample_file))
        self._content.direct_edit.setText(QtCore.QString(state.direct_beam))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = SampleData.DirectBeam()
        m.beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit)
        m.sample_file = unicode(self._content.sample_edit.text())
        m.direct_beam = unicode(self._content.direct_edit.text())
        return m
    
    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)      
        
    def _direct_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_edit.setText(fname)      
    
class BeamSpreader(BaseWidget):
    """
        Widget for the beam spreader transmission calculation options.
    """
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(BeamSpreader, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy) 
        
        class SpreaderFrame(QtGui.QGroupBox, ui.sans.ui_trans_spreader.Ui_GroupBox): 
            def __init__(self, parent=None):
                QtGui.QGroupBox.__init__(self, parent)
                self.setupUi(self)
                
        self._content = SpreaderFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(SampleData.BeamSpreader())      
  
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.spreader_trans_edit.setValidator(QtGui.QDoubleValidator(self._content.spreader_trans_edit))
        self._content.spreader_trans_spread_edit.setValidator(QtGui.QDoubleValidator(self._content.spreader_trans_spread_edit))
        
        # Connections
        self.connect(self._content.sample_scatt_browse, QtCore.SIGNAL("clicked()"), self._sample_scatt_browse)
        self.connect(self._content.sample_spread_browse, QtCore.SIGNAL("clicked()"), self._sample_spread_browse)
        self.connect(self._content.direct_scatt_browse, QtCore.SIGNAL("clicked()"), self._direct_scatt_browse)
        self.connect(self._content.direct_spread_browse, QtCore.SIGNAL("clicked()"), self._direct_spread_browse)

        self.connect(self._content.sample_scatt_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.sample_scatt_edit.text))
        self.connect(self._content.sample_spread_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.sample_spread_edit.text))
        self.connect(self._content.direct_scatt_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.direct_scatt_edit.text))
        self.connect(self._content.direct_spread_plot, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.direct_spread_edit.text))

        if not self._in_mantidplot:
            self._content.sample_scatt_plot.hide()
            self._content.sample_spread_plot.hide()
            self._content.direct_scatt_plot.hide()
            self._content.direct_spread_plot.hide()

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.spreader_trans_edit.setText(QtCore.QString(str(state.spreader_trans)))
        self._content.spreader_trans_spread_edit.setText(QtCore.QString(str(state.spreader_trans_spread)))
        self._content.sample_scatt_edit.setText(QtCore.QString(state.sample_scatt))
        self._content.sample_spread_edit.setText(QtCore.QString(state.sample_spreader))
        self._content.direct_scatt_edit.setText(QtCore.QString(state.direct_scatt))
        self._content.direct_spread_edit.setText(QtCore.QString(state.direct_spreader))
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = SampleData.BeamSpreader()
        m.spreader_trans = util._check_and_get_float_line_edit(self._content.spreader_trans_edit)
        m.spreader_trans_spread = util._check_and_get_float_line_edit(self._content.spreader_trans_spread_edit)
        m.sample_scatt = unicode(self._content.sample_scatt_edit.text())
        m.direct_scatt = unicode(self._content.direct_scatt_edit.text())
        m.sample_spreader = unicode(self._content.sample_spread_edit.text())
        m.direct_spreader = unicode(self._content.direct_spread_edit.text())
        return m    
    
    def _sample_scatt_plot(self):
         self.show_instrument(unicode(self._content.sample_scatt_edit.text()))
       
    def _sample_scatt_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_scatt_edit.setText(fname)      
        
    def _direct_scatt_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_scatt_edit.setText(fname)      
            
    def _sample_spread_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_spread_edit.setText(fname)      
        
    def _direct_spread_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_spread_edit.setText(fname)      
    
class SampleDataWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    _method_box = None

    ## Widget name
    name = "Sample"      
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(SampleDataWidget, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy) 

        class DataFrame(QtGui.QFrame, ui.sans.ui_hfir_sample_data.Ui_Frame): 
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
        self.connect(self._content.direct_beam_chk, QtCore.SIGNAL("clicked()"), self._direct_beam)
        self.connect(self._content.beam_spreader_chk, QtCore.SIGNAL("clicked()"), self._beam_spreader)
        self.connect(self._content.dark_current_button, QtCore.SIGNAL("clicked()"), self._dark_current_browse)
        
        self.connect(self._content.data_file_plot_button, QtCore.SIGNAL("clicked()"), self._data_file_plot)
        self.connect(self._content.dark_current_plot_button, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._content.dark_current_edit.text))

        if not self._in_mantidplot:
            self._content.dark_current_plot_button.hide()
            self._content.data_file_plot_button.hide()

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.transmission_edit.setText(QtCore.QString("%6.4f" % state.transmission))
        self._content.dtransmission_edit.setText(QtCore.QString("%6.4f" % state.transmission_spread))
        
        if isinstance(state.calculation_method, state.DirectBeam):
            self._content.direct_beam_chk.setChecked(True)
            self._direct_beam(state=state.calculation_method)
        else:
            self._content.beam_spreader_chk.setChecked(True)
            self._beam_spreader(state=state.calculation_method)

        self._content.calculate_chk.setChecked(state.calculate_transmission)
        self._content.theta_dep_chk.setChecked(state.theta_dependent)
        self._content.dark_current_edit.setText(QtCore.QString(str(state.dark_current)))
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
        m.dark_current = self._content.dark_current_edit.text()
        
        m.calculation_method=self._method_box.get_state()    
        
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

    def _data_file_plot(self):
        data_files = self._get_data_files()
        if len(data_files)>0:
            self.show_instrument(data_files[0])

    def _dark_current_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.dark_current_edit.setText(fname)      
            
    def _direct_beam(self, state=None):
        if state is None:
            state = self._last_direct_state
        if isinstance(self._method_box, BeamSpreader):
            self._last_spreader_state = self._method_box.get_state()
        self._replace_method(DirectBeam(self, state=state, settings=self._settings, 
                                        data_type=self._data_type, data_proxy=self._data_proxy))
        
    def _beam_spreader(self, state=None):
        if state is None:
            state = self._last_spreader_state
        if isinstance(self._method_box, DirectBeam):
            self._last_direct_state = self._method_box.get_state()
        self._replace_method(BeamSpreader(self, state=state, settings=self._settings, 
                                          data_type=self._data_type, data_proxy=self._data_proxy))
        
    def _replace_method(self, widget):
        if self._method_box is not None:
            for i in range(0, self._content.widget_placeholder.count()):
                item = self._content.widget_placeholder.itemAt(i)
                self._content.widget_placeholder.removeItem(self._content.widget_placeholder.itemAt(i))
                item.widget().deleteLater()
        self._method_box = widget
        self._content.widget_placeholder.addWidget(self._method_box)
        
    def _calculate_clicked(self, is_checked):
        self._content.direct_beam_chk.setEnabled(is_checked)
        self._content.beam_spreader_chk.setEnabled(is_checked)
        if self._method_box is not None:
            self._method_box.setEnabled(is_checked)
            
        self._content.transmission_edit.setEnabled(not is_checked)
        self._content.dtransmission_edit.setEnabled(not is_checked)
        
        self._content.dark_current_label.setEnabled(is_checked)
        self._content.dark_current_edit.setEnabled(is_checked)
        self._content.dark_current_button.setEnabled(is_checked)
        self._content.dark_current_plot_button.setEnabled(is_checked)

    def _get_data_files(self):
        """
            Utility method to return the list of data files in the sample data file edit box.
        """
        flist_str = str(self._content.data_file_edit.text())
        flist_str = flist_str.replace(',', ';')
        return flist_str.split(';')

    def _emit_experiment_parameters(self):
        sdd = util._check_and_get_float_line_edit(self._content.sample_dist_edit, min=0.0)
        self._settings.emit_key_value("sample_detector_distance", QtCore.QString(str(sdd)))
        wavelength = util._check_and_get_float_line_edit(self._content.wavelength_edit, min=0.0)
        self._settings.emit_key_value("wavelength", QtCore.QString(str(wavelength)))
        spread = self._content.wavelength_spread_edit.text()
        self._settings.emit_key_value("wavelength_spread", QtCore.QString(spread))
        
        
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
                self._content.sample_dist_edit.setText(QtCore.QString(str(dataproxy.sample_detector_distance)))
                util._check_and_get_float_line_edit(self._content.sample_dist_edit, min=0.0)
            if dataproxy.wavelength is not None:
                self._content.wavelength_edit.setText(QtCore.QString(str(dataproxy.wavelength)))
                util._check_and_get_float_line_edit(self._content.wavelength_edit, min=0.0)
            if dataproxy.wavelength_spread is not None:
                self._content.wavelength_spread_edit.setText(QtCore.QString(str(dataproxy.wavelength_spread)))
            if dataproxy.sample_thickness is not None:
                self._settings.emit_key_value("sample_thickness", QtCore.QString(str(dataproxy.sample_thickness)))
             
            self._emit_experiment_parameters()    