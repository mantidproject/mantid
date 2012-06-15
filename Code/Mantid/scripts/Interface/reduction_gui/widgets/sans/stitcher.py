from PyQt4 import QtGui, uic, QtCore
import os
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util
import ui.ui_stitcher

import mantidplot
from MantidFramework import *
mtd.initialise(False)
from mantidsimple import *

from LargeScaleStructures.data_stitching import DataSet, Stitcher, RangeSelector

from reduction_gui.reduction.scripter import BaseScriptElement
class StitcherState(BaseScriptElement):
    def __init__(self):
        pass
    
class StitcherWidget(BaseWidget):    
    """
        Widget that present a data catalog to the user
    """
    ## Widget name
    name = "Data Stitching"      
         
    def __init__(self, parent=None, state=None, settings=None):      
        super(StitcherWidget, self).__init__(parent, state, settings) 

        class DataFrame(QtGui.QFrame, ui.ui_stitcher.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DataFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._content)
                
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

       # Connect do UI data update
        self._settings.data_updated.connect(self._data_updated)
        
        self._low_q_data = None
        self._medium_q_data = None
        self._high_q_data = None
        
        self._referenceID = 0
        
        self._graph = "StitchedData"
        self._output_dir = None
        self._stitcher = None
        self._plotted = False

    def initialize_content(self):
        """
            Initialize the content of the frame
        """
        # Validators
        self._content.low_scale_edit.setValidator(QtGui.QDoubleValidator(self._content.low_scale_edit))
        self._content.medium_scale_edit.setValidator(QtGui.QDoubleValidator(self._content.medium_scale_edit))
        self._content.high_scale_edit.setValidator(QtGui.QDoubleValidator(self._content.high_scale_edit))

        self._content.low_min_edit.setValidator(QtGui.QDoubleValidator(self._content.low_min_edit))
        self._content.low_max_edit.setValidator(QtGui.QDoubleValidator(self._content.low_max_edit))
        self._content.medium_min_edit.setValidator(QtGui.QDoubleValidator(self._content.medium_min_edit))
        self._content.medium_max_edit.setValidator(QtGui.QDoubleValidator(self._content.medium_max_edit))

        # Browse buttons
        self.connect(self._content.low_q_browse_button, QtCore.SIGNAL("clicked()"), self._low_q_browse)
        self.connect(self._content.medium_q_browse_button, QtCore.SIGNAL("clicked()"), self._medium_q_browse)
        self.connect(self._content.high_q_browse_button, QtCore.SIGNAL("clicked()"), self._high_q_browse)
        
        self.connect(self._content.low_q_combo, QtCore.SIGNAL("activated(int)"), self._update_low_q)
        self.connect(self._content.medium_q_combo, QtCore.SIGNAL("activated(int)"), self._update_medium_q)
        self.connect(self._content.high_q_combo, QtCore.SIGNAL("activated(int)"), self._update_high_q)
        
        # Radio buttons
        self.connect(self._content.low_radio, QtCore.SIGNAL("clicked()"), self._low_q_selected)
        self.connect(self._content.medium_radio, QtCore.SIGNAL("clicked()"), self._medium_q_selected)
        self.connect(self._content.high_radio, QtCore.SIGNAL("clicked()"), self._high_q_selected)

        # Selection buttons
        self.connect(self._content.low_range_button, QtCore.SIGNAL("clicked()"), self._low_range)
        self.connect(self._content.medium_range_button, QtCore.SIGNAL("clicked()"), self._medium_range)
        
        # Scale factors
        self.connect(self._content.low_scale_edit, QtCore.SIGNAL("returnPressed()"), self._update_low_scale)
        self.connect(self._content.medium_scale_edit, QtCore.SIGNAL("returnPressed()"), self._update_medium_scale)
        self.connect(self._content.high_scale_edit, QtCore.SIGNAL("returnPressed()"), self._update_high_scale)
        
        # Apply and save buttons
        self.connect(self._content.apply_button, QtCore.SIGNAL("clicked()"), self._apply)        
        self.connect(self._content.save_result_button, QtCore.SIGNAL("clicked()"), self._save_result)
        
        # Create button group for data set selection
        g = QtGui.QButtonGroup(self)
        g.addButton(self._content.low_radio)
        g.addButton(self._content.medium_radio)
        g.addButton(self._content.high_radio)
        g.setExclusive(True)
        self._content.low_radio.setChecked(True)
        
        self._content.low_q_combo.insertItem(0,"")
        self.populate_combobox(self._content.low_q_combo)
        self._content.low_q_combo.setEditable(True)
        
        self._content.medium_q_combo.insertItem(0,"")
        self.populate_combobox(self._content.medium_q_combo)
        self._content.medium_q_combo.setEditable(True)
        
        self._content.high_q_combo.insertItem(0,"")
        self.populate_combobox(self._content.high_q_combo)
        self._content.high_q_combo.setEditable(True)
        
        class ShowEventFilter(QtCore.QObject):
            def eventFilter(obj_self, filteredObj, event):
                if event.type() == QtCore.QEvent.HoverEnter:
                    self.populate_combobox(filteredObj)
                    filteredObj.update()
                elif event.type() == QtCore.QEvent.KeyPress:
                    if event.key() == QtCore.Qt.Key_Return or event.key() == QtCore.Qt.Key_Enter:
                        filteredObj.setItemText(0, filteredObj.lineEdit().text())
                        if filteredObj==self._content.low_q_combo:
                            self._update_low_q()
                        elif filteredObj==self._content.medium_q_combo:
                            self._update_medium_q()
                        elif filteredObj==self._content.high_q_combo:
                            self._update_high_q()
                        return True
                    
                return QtCore.QObject.eventFilter(obj_self, filteredObj, event)
        
        eventFilter = ShowEventFilter(self)
        self._content.low_q_combo.installEventFilter(eventFilter)
        self._content.medium_q_combo.installEventFilter(eventFilter)
        self._content.high_q_combo.installEventFilter(eventFilter)

    def populate_combobox(self, combo):
        ws_list = mtd.getWorkspaceNames()
        for ws in ws_list:
            if not ws.startswith("__") and combo.findText(ws)<0\
             and hasattr(mtd[ws], "getNumberHistograms") and  mtd[ws].getNumberHistograms()==1:
                combo.addItem(ws)
        
    def _update_low_scale(self):
        """
            Callback for scale update from user
        """
        self._low_q_data.set_scale(float(self._content.low_scale_edit.text()))
        self.plot_result()

    def _update_medium_scale(self):
        """
            Callback for scale update from user
        """
        self._medium_q_data.set_scale(float(self._content.medium_scale_edit.text()))
        self.plot_result()

    def _update_high_scale(self):
        """
            Callback for scale update from user
        """
        self._high_q_data.set_scale(float(self._content.high_scale_edit.text()))
        self.plot_result()
    
    def _low_range(self):
        """
            User requested to select range common to data sets 1 and 2
        """
        if self._low_q_data is not None:
            def call_back(xmin, xmax):
                self._content.low_min_edit.setText("%-6.3g" % xmin)
                self._content.low_max_edit.setText("%-6.3g" % xmax)
            ws_list = []
            if self._low_q_data is not None:
                ws_list.append(str(self._low_q_data))
            if self._medium_q_data is not None:
                ws_list.append(str(self._medium_q_data))
            RangeSelector.connect(ws_list, call_back=call_back)
                
    def _medium_range(self):
        """
            User requested to select range common to data sets 2 and 3
        """
        if self._medium_q_data is not None:
            def call_back(xmin, xmax):
                self._content.medium_min_edit.setText("%-6.3g" % xmin)
                self._content.medium_max_edit.setText("%-6.3g" % xmax)
            ws_list = []
            if self._medium_q_data is not None:
                ws_list.append(str(self._medium_q_data))
            if self._high_q_data is not None:
                ws_list.append(str(self._high_q_data))
            RangeSelector.connect(ws_list, call_back=call_back)
        
    def _low_q_selected(self):
        """
            Callback for radio button clicked [selected as reference data set]
        """
        self._content.low_scale_edit.setText("1.0")
        self._referenceID = 0
        
    def _medium_q_selected(self):
        """
            Callback for radio button clicked [selected as reference data set]
        """
        self._content.medium_scale_edit.setText("1.0")
        self._referenceID = 1
        
    def _high_q_selected(self):
        """
            Callback for radio button clicked [selected as reference data set]
        """
        self._content.high_scale_edit.setText("1.0")
        self._referenceID = 2
        
    def update_data(self, dataset_control, min_control, max_control,
                    scale_control, first_spin_control, last_spin_control):
        """
            Update a data set
            
            @param dataset_control: combo box with the file path or workspace name
            @param min_control: text widget containing the minimum Q of the overlap region
            @param max_control: text widget containing the maximum Q of the overlap region
            @param scale_control: text widget containing the scale (can be input or output)
            @param first_spin_control: spinner widget containing the number of points to skip at the beginning
            @param last_spin_control: spinner widget containing the number of points to skip at the end 
        """
        data_object = None
        
        file = str(dataset_control.lineEdit().text())
        if len(file.strip())==0:
            data_object = None
        elif os.path.isfile(file) or mtd.workspaceExists(file):
            data_object = DataSet(file)
            try:
                data_object.load(True)
            except:
                data_object = None
                util.set_valid(dataset_control.lineEdit(), False)
                QtGui.QMessageBox.warning(self, "Error loading file", "Could not load %s.\nMake sure you pick the XML output from the reduction." % file)
                return
            if min_control is not None and max_control is not None \
                and (len(min_control.text())==0 or len(max_control.text())==0):
                minx, maxx = data_object.get_range()
                min_control.setText("%-6.3g" % minx)
                max_control.setText("%-6.3g" % maxx)
            
            # Set the reference scale, unless we just loaded the data
            if len(scale_control.text())==0:
                scale_control.setText("1.0")
            else:
                scale = util._check_and_get_float_line_edit(scale_control)
                data_object.set_scale(scale)
                
            npts = data_object.get_number_of_points()
            first_spin_control.setMaximum(npts)
            last_spin_control.setMaximum(npts)
            util.set_valid(dataset_control.lineEdit(), True)
        else:
            data_object = None
            util.set_valid(dataset_control.lineEdit(), False)
        self._plotted = False
        
        return data_object
        
    def _update_low_q(self, ws=None):
        """
            Update Low-Q data set
        """
        self._low_q_data = self.update_data(self._content.low_q_combo,
                                            self._content.low_min_edit,
                                            self._content.low_max_edit,
                                            self._content.low_scale_edit,
                                            self._content.low_first_spin,
                                            self._content.low_last_spin)

    def _update_medium_q(self, ws=None):
        """
            Update Medium-Q data set
        """
        self._medium_q_data = self.update_data(self._content.medium_q_combo,
                                               self._content.medium_min_edit,
                                               self._content.medium_max_edit,
                                               self._content.medium_scale_edit,
                                               self._content.medium_first_spin,
                                               self._content.medium_last_spin)

    def _update_high_q(self, ws=None):
        """
            Update High-Q data set
        """
        self._high_q_data = self.update_data(self._content.high_q_combo,
                                             None,
                                             None,
                                             self._content.high_scale_edit,
                                             self._content.high_first_spin,
                                             self._content.high_last_spin)

        file = str(self._content.high_q_combo.lineEdit().text())
        if len(file.strip())==0:
            self._high_q_data = None
        elif os.path.isfile(file) or mtd.workspaceExists(file):
            self._high_q_data = DataSet(file)
            try:
                self._high_q_data.load(True)
            except:
                self._high_q_data = None
                util.set_valid(self._content.high_q_combo.lineEdit(), False)
                QtGui.QMessageBox.warning(self, "Error loading file", "Could not load %s.\nMake sure you pick the XML output from the reduction." % file)
                return                
            self._content.high_scale_edit.setText("1.0")
            npts = self._high_q_data.get_number_of_points()
            self._content.high_first_spin.setMaximum(npts)
            self._content.high_last_spin.setMaximum(npts)
            util.set_valid(self._content.high_q_combo.lineEdit(), True)
        else:
            self._high_q_data = None
            util.set_valid(self._content.high_q_combo.lineEdit(), False)
        self._plotted = False

    def data_browse_dialog(self):
        """
            Pop up a file dialog box.
        """
        title = "Data file - Choose a reduced I(Q) file"
        if not os.path.isdir(str(self._output_dir)):
            self._output_dir = os.path.expanduser("~")
        fname = QtCore.QFileInfo(QtGui.QFileDialog.getOpenFileName(self, title,
                                                                   self._output_dir,
                                                                   "Reduced XML files (*.xml);; Reduced Nexus files (*.nxs);; All files (*.*)")).filePath()
        if fname:
            # Store the location of the loaded file
            self._output_dir = str(QtCore.QFileInfo(fname).path())
        return str(fname)     
 
    def _low_q_browse(self):
        """
            Browse for Low-Q I(Q) data set 
        """
        fname = self.data_browse_dialog()
        if fname:
            self._content.low_q_combo.setItemText(0,fname)
            self._content.low_q_combo.setCurrentIndex(0)
            self._update_low_q()

    def _medium_q_browse(self):
        """
            Browse for Medium-Q I(Q) data set 
        """
        fname = self.data_browse_dialog()
        if fname:
            self._content.medium_q_combo.setItemText(0,fname)
            self._content.medium_q_combo.setCurrentIndex(0)
            self._update_medium_q()

    def _high_q_browse(self):
        """
            Browse for High-Q I(Q) data set 
        """
        fname = self.data_browse_dialog()
        if fname:
            self._content.high_q_combo.setItemText(0,fname)
            self._content.high_q_combo.setCurrentIndex(0)
            self._update_high_q()   

    def is_running(self, is_running):
        """
            Enable/disable controls depending on whether a reduction is running or not
            @param is_running: True if a reduction is running
        """
        super(StitcherWidget, self).is_running(is_running)
        self._content.save_result_button.setEnabled(not is_running)
        self._content.apply_button.setEnabled(not is_running)
        
    def _data_updated(self, key, value):
        """
            Respond to application-level key/value pair updates.
            @param key: key string
            @param value: value string
        """
        if key=="OUTPUT_DIR":
            self._output_dir = value
        
    def _apply(self):
        """
            Perform auto-scaling
        """
        # Update data sets, in case the user typed in a file name without using the browse button
        self._update_low_q()
        self._update_medium_q()
        self._update_high_q()
        
        s = Stitcher()
        if self._low_q_data is not None:
            xmin = util._check_and_get_float_line_edit(self._content.low_min_edit)
            xmax = util._check_and_get_float_line_edit(self._content.low_max_edit)
            self._low_q_data.set_range(xmin,xmax)
            s.append(self._low_q_data)
            if self._referenceID==0:
                scale = util._check_and_get_float_line_edit(self._content.low_scale_edit)
                self._low_q_data.set_scale(scale)
            
        if self._medium_q_data is not None:
            s.append(self._medium_q_data)
            if self._referenceID==1:
                scale = util._check_and_get_float_line_edit(self._content.medium_scale_edit)
                self._medium_q_data.set_scale(scale)
                        
        if self._high_q_data is not None:
            xmin = util._check_and_get_float_line_edit(self._content.medium_min_edit)
            xmax = util._check_and_get_float_line_edit(self._content.medium_max_edit)
            self._high_q_data.set_range(xmin,xmax)
            s.append(self._high_q_data)
            if self._referenceID==2:
                scale = util._check_and_get_float_line_edit(self._content.high_scale_edit)
                self._high_q_data.set_scale(scale)
        
        if s.size()==0:
            return
        
        s.set_reference(self._referenceID)
        s.compute()

        # Update scaling factor
        if self._low_q_data is not None:
            self._content.low_scale_edit.setText(str(self._low_q_data.get_scale()))
            n_first = util._check_and_get_int_line_edit(self._content.low_first_spin)
            n_last = util._check_and_get_int_line_edit(self._content.low_last_spin)
            self._low_q_data.set_skipped_points(n_first, n_last)
        if self._medium_q_data is not None:
            self._content.medium_scale_edit.setText(str(self._medium_q_data.get_scale()))
            n_first = util._check_and_get_int_line_edit(self._content.medium_first_spin)
            n_last = util._check_and_get_int_line_edit(self._content.medium_last_spin)
            self._medium_q_data.set_skipped_points(n_first, n_last)
        if self._high_q_data is not None:
            self._content.high_scale_edit.setText(str(self._high_q_data.get_scale()))
            n_first = util._check_and_get_int_line_edit(self._content.high_first_spin)
            n_last = util._check_and_get_int_line_edit(self._content.high_last_spin)
            self._high_q_data.set_skipped_points(n_first, n_last)
        
        self._stitcher = s
        
        self.plot_result()
        
    def plot_result(self):
        """
            Plot the scaled data sets
        """
        ws_list = []
        if self._low_q_data is not None:
            self._low_q_data.apply_scale()
            ws_list.append(self._low_q_data.get_scaled_ws())
        
        if self._medium_q_data is not None:
            self._medium_q_data.apply_scale()
            ws_list.append(self._medium_q_data.get_scaled_ws())
        
        if self._high_q_data is not None:
            self._high_q_data.apply_scale()
            ws_list.append(self._high_q_data.get_scaled_ws())
        
        if len(ws_list)>0:
            g = mantidplot.graph(self._graph)
            if g is None or not self._plotted:
                g = mantidplot.plotSpectrum(ws_list, [0], True)
                g.setName(self._graph)
                self._plotted = True
                
    def _save_result(self):
        """
            Save the scaled output in one combined I(Q) file
        """
        if self._stitcher is not None:
            if not os.path.isdir(self._output_dir):
                self._output_dir = os.path.expanduser("~")
            fname_qstr = QtGui.QFileDialog.getSaveFileName(self, "Save combined I(Q)",
                                                           self._output_dir, 
                                                           "Data Files (*.xml)")
            fname = str(QtCore.QFileInfo(fname_qstr).filePath())
            if len(fname)>0:
                if fname.endswith('.xml'):
                    self._stitcher.save_combined(fname, as_canSAS=True)
                elif fname.endswith('.txt'):
                    self._stitcher.save_combined(fname, as_canSAS=False)
                else:
                    fname_tmp = fname + ".xml"
                    self._stitcher.save_combined(fname_tmp, as_canSAS=True)
                    fname_tmp = fname + ".txt"
                    self._stitcher.save_combined(fname_tmp, as_canSAS=False)
    
    def set_state(self, state):
        """
            Update the catalog according to the new data path
        """
        # Refresh combo boxes
        self.populate_combobox(self._content.low_q_combo)
        self.populate_combobox(self._content.medium_q_combo)
        self.populate_combobox(self._content.high_q_combo)

    def get_state(self):
        """
            Return dummy state
        """
        return StitcherState()