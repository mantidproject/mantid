from PyQt4 import QtGui, uic, QtCore
import sip
import os
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util
import ui.reflectometer.ui_refl_stitching

import _qti
from MantidFramework import *
mtd.initialise(False)
from mantidsimple import *

from LargeScaleStructures.data_stitching import DataSet, Stitcher, RangeSelector

from reduction_gui.reduction.scripter import BaseScriptElement
class StitcherState(BaseScriptElement):
    def __init__(self):
        pass
    
class ReflData(object):
    name = ""
    scale = 1.0
    OFF_OFF = 0
    OFF_ON  = 1
    ON_OFF  = 2
    ON_ON   = 3
    
    def __init__(self, workspace, is_ref=False, scale=1.0, parent_layout=None):
        self.name = workspace
        self._scale = scale
        self._data = [None, None, None, None]
        self._call_back = None
        
        # Widgets
        self._layout = QtGui.QHBoxLayout()

        self._label = QtGui.QLabel(workspace)
        self._label.setMinimumSize(QtCore.QSize(250, 0))
        self._label.setMaximumSize(QtCore.QSize(250, 16777215))
        
        self._radio = QtGui.QRadioButton()
        self._edit_ctrl = QtGui.QLineEdit()
        self._edit_ctrl.setMinimumSize(QtCore.QSize(80, 0))
        self._edit_ctrl.setMaximumSize(QtCore.QSize(80, 16777215))
        self._edit_ctrl.setValidator(QtGui.QDoubleValidator(self._edit_ctrl))
        

        self._spacer = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self._layout.addWidget(self._radio)
        self._layout.addWidget(self._label)
        self._layout.addItem(self._spacer)
        self._layout.addWidget(self._edit_ctrl)
        self._edit_ctrl.setText(str(self._scale))
        
        if parent_layout is not None:
            parent_layout.addLayout(self._layout)
            parent_layout.connect(self._edit_ctrl, QtCore.SIGNAL("returnPressed()"), self._scale_updated)
            #parent_layout.connect(self._radio, QtCore.SIGNAL("toggled()"), self._reference_updated)
        
    def is_selected(self):
        return self._radio.isChecked()
    
    def _scale_updated(self):
        for item in self._data:
            if item is not None:
                try:
                    self._scale = float(self._edit_ctrl.text())
                    item.set_scale(self._scale)
                    item.apply_scale()
                except:
                    pass
            
        if self._call_back is not None:
            self._call_back()
            
    def delete(self):
        if self._radio is not None:
            sip.delete(self._radio)
        if self._label is not None:
            sip.delete(self._label)
        if self._edit_ctrl is not None:
            sip.delete(self._edit_ctrl)
        if self._layout is not None:
            sip.delete(self._layout)

    def select(self):
        self._radio.setChecked(True)
        
    def get_scale(self):
        return self._scale
    
    def set_scale(self, scale):
        value_as_string = "%-6.3g" % scale
        self._edit_ctrl.setText(value_as_string.strip())
        self._scale = scale
        
        # Update all polarization states
        self._scale_updated()
    
    def set_user_data(self, data):
        self._data[ReflData.OFF_OFF] = data
        
        if self.name.find("Off_Off")>0:
            ws_name = self.name.replace("Off_Off", "On_Off")
            if mtd.workspaceExists(ws_name):
                self._data[ReflData.ON_OFF] = DataSet(ws_name)
                self._data[ReflData.ON_OFF].load(True, True)
    
            ws_name = self.name.replace("Off_Off", "Off_On")
            if mtd.workspaceExists(ws_name):
                self._data[ReflData.OFF_ON] = DataSet(ws_name)
                self._data[ReflData.OFF_ON].load(True, True)
    
            ws_name = self.name.replace("Off_Off", "On_On")
            if mtd.workspaceExists(ws_name):
                self._data[ReflData.ON_ON] = DataSet(ws_name)
                self._data[ReflData.ON_ON].load(True, True)
    
    def get_user_data(self, polarization=0):
        if polarization in [0,1,2,3]:
            return self._data[polarization]
        else: 
            return self._data[ReflData.OFF_OFF]
    
    def connect_to_scale(self, call_back):
        self._call_back = call_back
    
class StitcherWidget(BaseWidget):    
    """
        Widget that present a data catalog to the user
    """
    ## Widget name
    name = "Data Stitching"      
         
    def __init__(self, parent=None, state=None, settings=None):      
        super(StitcherWidget, self).__init__(parent, state, settings) 

        class DataFrame(QtGui.QFrame, ui.reflectometer.ui_refl_stitching.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DataFrame(self)
        self._layout.addWidget(self._content)
                
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings
        
        self._graph = "Stitched Data"
        self._output_dir = None
        self._stitcher = None
        self._data_sets = []
        
        self._workspace_list = []
        self.initialize_content()

    def initialize_content(self):
        """
            Initialize the content of the frame
        """
        # Apply and save buttons
        self.connect(self._content.auto_scale_btn, QtCore.SIGNAL("clicked()"), self._apply)        
        self.connect(self._content.save_btn, QtCore.SIGNAL("clicked()"), self._save_result)
        
    def _add_entry(self, workspace):
        entry = ReflData(workspace, parent_layout=self._content.angle_list_layout)
        self._workspace_list.append(entry)

    def is_running(self, is_running):
        """
            Enable/disable controls depending on whether a reduction is running or not
            @param is_running: True if a reduction is running
        """
        super(StitcherWidget, self).is_running(is_running)
        self._content.save_btn.setEnabled(not is_running)
        self._content.auto_scale_btn.setEnabled(not is_running)
        
    def _load_workspace(self, workspace):
        ws_data = DataSet(workspace)
        try:
            ws_data.load(True, True)
        except:
            ws_data = None
            QtGui.QMessageBox.warning(self, "Error loading file", "Could not load %s." % file)
            return

    def _apply(self):
        """
            Perform auto-scaling
        """
        s = Stitcher()
        refID = 0
        for i in range(len(self._workspace_list)):
            item = self._workspace_list[i]
            data = DataSet(item.name)
            data.load(True, True)
            xmin,xmax = data.get_range()
            item.set_user_data(data)

            if item.is_selected():
                data.set_scale(item.get_scale())
                refID = i
            
            s.append(data)
        
        if s.size()==0:
            return
        
        s.set_reference(refID)
        s.compute()
        
        for item in self._workspace_list:
            data = item.get_user_data()
            data.apply_scale()
            scale = data.get_scale()
            item.set_scale(scale)

        self._stitcher = s
        
        self.plot_result()
        
    def plot_result(self, polarization=None):
        """
            Plot the scaled data sets
        """
        self._stitcher.get_scaled_data()
        
        pol_dict = {"Off Off" : ReflData.OFF_OFF,
                    "On Off" : ReflData.ON_OFF,
                    "Off On" : ReflData.OFF_ON,
                    "On On"  : ReflData.ON_ON }
        
        for pol in pol_dict.keys():
            ws_list = []
            for item in self._workspace_list:
                d = item.get_user_data( pol_dict[pol] )
                if d is not None:
                    ws_list.append(d.get_scaled_ws())
            
            if len(ws_list)>0:
                plot_name = '%s: %s' % (self._graph, pol)
                g = _qti.app.graph(plot_name)
                if g is not None:
                    g.close()
                g = _qti.app.mantidUI.pyPlotSpectraList(ws_list,[0],True)
                g.setName(plot_name)
                l=g.activeLayer()
                l.setTitle("Polarization state: %s" % pol)
                
    def _save_result(self):
        """
            Save the scaled output in one combined I(Q) file
        """
        if self._stitcher is not None:
            if self._output_dir is None or not os.path.isdir(self._output_dir):
                self._output_dir = os.path.expanduser("~")
            fname_qstr = QtGui.QFileDialog.getSaveFileName(self, "Save combined I(Q)",
                                                           self._output_dir, 
                                                           "Data Files (*.txt)")
            fname = str(QtCore.QFileInfo(fname_qstr).filePath())
            if len(fname)>0:
                self._stitcher.save_combined(fname, as_canSAS=False)
    
    def set_state(self, state):
        """
            Update the catalog according to the new data path
        """
        # Remove current entries
        for item in self._workspace_list:
            item.delete()
            
        self._workspace_list = []
        
        # Refresh combo boxes
        for item in mtd.keys():
            if item.startswith("reflectivity") and not item.endswith("scaled")\
            and item.find('On_Off')<0 and item.find('Off_On')<0\
            and item.find('On_On')<0:
                self._add_entry(item)
                
        if len(self._workspace_list)>0:
            self._workspace_list[0].select()
            self._apply()

    def get_state(self):
        """
            Return dummy state
        """
        return StitcherState()