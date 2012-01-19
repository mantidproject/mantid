from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import _qti
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets
from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_data_refl_simple

from reduction.instruments.reflectometer import data_manipulation

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from MantidFramework import *
    mtd.initialise(False)
    from mantidsimple import *
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

        # Event connections
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.plot_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y)
        self.connect(self._summary.plot_tof_btn, QtCore.SIGNAL("clicked()"), self._plot_tof)
        self.connect(self._summary.add_dataset_btn, QtCore.SIGNAL("clicked()"), self._add_data)
        self.connect(self._summary.angle_list, QtCore.SIGNAL("itemSelectionChanged()"), self._angle_changed)

    def _data_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.data_background_from_pixel1.setEnabled(is_checked)
        self._summary.data_background_from_pixel1_label.setEnabled(is_checked)
        self._summary.data_background_to_pixel1.setEnabled(is_checked)
        self._summary.data_background_to_pixel1_label.setEnabled(is_checked)
        
    def _plot_count_vs_y(self):
        f = FileFinder.findRuns("REF_L%s" % str(self._summary.data_run_number_edit.text()))
        if len(f)>0 and os.path.isfile(f[0]):
            data_manipulation.counts_vs_y_distribution(f[0], 9000, 23600)

    def _plot_tof(self):
        f = FileFinder.findRuns("REF_L%s" % str(self._summary.norm_run_number_edit.text()))
        if len(f)>0 and os.path.isfile(f[0]):
            data_manipulation.tof_distribution(f[0])

    def _add_data(self):
        state = self.get_editing_state()
        # Check whether it's already in the list
        run_numbers = self._summary.data_run_number_edit.text()
        list_items = self._summary.angle_list.findItems(run_numbers, QtCore.Qt.MatchFixedString)
        if len(list_items)>0:
            print "Found"
            list_items[0].setData(QtCore.Qt.UserRole, state)
        else:
            item_widget = QtGui.QListWidgetItem(run_numbers, self._summary.angle_list)
            item_widget.setData(QtCore.Qt.UserRole, state)

    def _angle_changed(self):
        state = self._summary.angle_list.currentItem().data(QtCore.Qt.UserRole).toPyObject()
        self.set_editing_state(state)

    def _check_for_missing_fields(self):

        self._summary.data_peak_discrete_selection_missing.setText(" ")
        from_pixel = self._summary.data_peak_from_pixel.text()
        if from_pixel == '':
            self._summary.data_peak_from_pixel_missing.setText("*")
        else:
            self._summary.data_peak_from_pixel_missing.setText(" ")
            
        to_pixel = self._summary.data_peak_to_pixel.text()
        if to_pixel == '':
            self._summary.data_peak_to_pixel_missing.setText("*")
        else:
            self._summary.data_peak_to_pixel_missing.setText(" ")
            
        #background
        is_checked = self._summary.data_background_switch.isChecked()
        if is_checked:
            from_pixel1 = self._summary.data_background_from_pixel1.text()
            if from_pixel1 == '':
                self._summary.data_background_from_pixel_missing.setText("*")
            else:
                self._summary.data_background_from_pixel_missing.setText(" ")
                
            to_pixel1 = self._summary.data_background_to_pixel1.text()
            if to_pixel1 == '':
                self._summary.data_background_to_pixel_missing.setText("*")
            else:
                self._summary.data_background_to_pixel_missing.setText(" ")
            
        else:
            self._summary.data_background_from_pixel_missing.setText(" ")
            self._summary.data_background_to_pixel_missing.setText(" ")
                                    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object    
        """
        if IS_IN_MANTIDPLOT:
            ws_name = "reflectivity"
            ws_list = [n for n in mtd.keys() if n.startswith(ws_name)]
            g = _qti.app.graph(ws_name)
            if g is None and len(ws_list)>0:
                g = _qti.app.mantidUI.pyPlotSpectraList(ws_list,[0],True)
                g.setName(ws_name)  
        
        self._summary.angle_list.clear()
        for item in state.data_sets:
            if item is not None:
                item_widget = QtGui.QListWidgetItem(unicode(str(','.join([str(i) for i in item.data_files]))), self._summary.angle_list)
                item_widget.setData(QtCore.Qt.UserRole, item)

        self.set_editing_state(state.data_sets[0])

    def set_editing_state(self, state):

        #Peak from/to pixels
        self._summary.data_peak_from_pixel.setText(str(state.DataPeakPixels[0]))
        self._summary.data_peak_to_pixel.setText(str(state.DataPeakPixels[1]))
        
        self._summary.x_min_edit.setText(str(state.x_range[0]))
        self._summary.x_max_edit.setText(str(state.x_range[1]))
        
        #Background flag
        self._summary.data_background_switch.setChecked(state.DataBackgroundFlag)

        #Background from/to pixels
        self._summary.data_background_from_pixel1.setText(str(state.DataBackgroundRoi[0]))
        self._summary.data_background_to_pixel1.setText(str(state.DataBackgroundRoi[1]))
        
        #from TOF and to TOF
        self._summary.data_from_tof.setText(str(state.DataTofRange[0]))
        self._summary.data_to_tof.setText(str(state.DataTofRange[1]))
        
        self._summary.data_run_number_edit.setText(str(','.join([str(i) for i in state.data_files])))
        
        # Normalization options
        self._summary.norm_run_number_edit.setText(str(state.norm_file))
        self._summary.norm_peak_from_pixel.setText(str(state.NormPeakPixels[0]))
        self._summary.norm_peak_to_pixel.setText(str(state.NormPeakPixels[1]))
        
        self._summary.norm_background_switch.setChecked(state.NormBackgroundFlag)

        self._summary.norm_background_from_pixel1.setText(str(state.NormBackgroundRoi[0]))
        self._summary.norm_background_to_pixel1.setText(str(state.NormBackgroundRoi[1]))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = self.get_editing_state()
        state = DataSeries()
        state_list = []
        for i in range(self._summary.angle_list.count()):
            data = self._summary.angle_list.item(i).data(QtCore.Qt.UserRole).toPyObject()
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
        return m