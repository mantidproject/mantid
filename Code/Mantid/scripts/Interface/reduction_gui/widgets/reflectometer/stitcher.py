#pylint: disable=invalid-name
from PyQt4 import QtGui, uic, QtCore
import sip
import os
import sys
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util
import ui.reflectometer.ui_refl_stitching

import mantidplot
from mantid.simpleapi import *
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
        self._radio.setMinimumSize(QtCore.QSize(20, 0))
        self._radio.setMaximumSize(QtCore.QSize(20, 16777215))
        self._edit_ctrl = QtGui.QLineEdit()
        self._edit_ctrl.setMinimumSize(QtCore.QSize(80, 0))
        self._edit_ctrl.setMaximumSize(QtCore.QSize(80, 16777215))
        self._edit_ctrl.setValidator(QtGui.QDoubleValidator(self._edit_ctrl))

        self._low_skip_ctrl = QtGui.QLineEdit()
        self._low_skip_ctrl.setMinimumSize(QtCore.QSize(80, 0))
        self._low_skip_ctrl.setMaximumSize(QtCore.QSize(80, 16777215))
        self._low_skip_ctrl.setValidator(QtGui.QIntValidator(self._low_skip_ctrl))

        self._high_skip_ctrl = QtGui.QLineEdit()
        self._high_skip_ctrl.setMinimumSize(QtCore.QSize(80, 0))
        self._high_skip_ctrl.setMaximumSize(QtCore.QSize(80, 16777215))
        self._high_skip_ctrl.setValidator(QtGui.QIntValidator(self._high_skip_ctrl))

        self._spacer = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self._layout.addWidget(self._radio)
        self._layout.addWidget(self._label)
        self._layout.addItem(self._spacer)
        self._layout.addWidget(self._edit_ctrl)
        self._edit_ctrl.setText(str(self._scale))

        self._layout.addWidget(self._low_skip_ctrl)
        self._low_skip_ctrl.setText("0")
        self._layout.addWidget(self._high_skip_ctrl)
        self._high_skip_ctrl.setText("0")

        if parent_layout is not None:
            parent_layout.addLayout(self._layout)
            parent_layout.connect(self._edit_ctrl, QtCore.SIGNAL("returnPressed()"), self._return_pressed)
            #parent_layout.connect(self._radio, QtCore.SIGNAL("toggled()"), self._reference_updated)

    def is_selected(self):
        return self._radio.isChecked()

    def get_common_range(self):
        """
            Get common range between all polarization states,
            excluding beginning and trailing zeros
        """
        # Get valid range
        xmin = None
        xmax = None
        for item in self._data:
            if item is not None:
                _xmin, _xmax = item.get_skipped_range()
                if xmin is None or _xmin>xmin:
                    xmin = _xmin
                if xmax is None or _xmax<xmax:
                    xmax = _xmax
        return xmin, xmax

    def _return_pressed(self):
        self._scale_updated()
        if self._call_back is not None:
            self._call_back()

    def _scale_updated(self):
        """
            Called when the scaling factors are updated
        """
        xmin, xmax = self.get_common_range()
        for item in self._data:
            if item is not None:
                try:
                    self._scale = float(self._edit_ctrl.text())
                    item.set_scale(self._scale)
                    item.apply_scale(xmin=xmin, xmax=xmax)
                except:
                    pass

    def delete(self):
        if self._radio is not None:
            sip.delete(self._radio)
        if self._label is not None:
            sip.delete(self._label)
        if self._edit_ctrl is not None:
            sip.delete(self._edit_ctrl)
        if self._low_skip_ctrl is not None:
            sip.delete(self._low_skip_ctrl)
        if self._high_skip_ctrl is not None:
            sip.delete(self._high_skip_ctrl)
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
            if AnalysisDataService.doesExist(ws_name):
                self._data[ReflData.ON_OFF] = DataSet(ws_name)
                self._data[ReflData.ON_OFF].load(True, True)

            ws_name = self.name.replace("Off_Off", "Off_On")
            if AnalysisDataService.doesExist(ws_name):
                self._data[ReflData.OFF_ON] = DataSet(ws_name)
                self._data[ReflData.OFF_ON].load(True, True)

            ws_name = self.name.replace("Off_Off", "On_On")
            if AnalysisDataService.doesExist(ws_name):
                self._data[ReflData.ON_ON] = DataSet(ws_name)
                self._data[ReflData.ON_ON].load(True, True)

    def get_user_data(self, polarization=0):
        if polarization in [0,1,2,3]:
            return self._data[polarization]
        else:
            return self._data[ReflData.OFF_OFF]

    def get_skipped(self):
        low_str = str(self._low_skip_ctrl.text())
        low = 0
        high = 0
        if len(low_str.strip())>0:
            low = int(low_str)
        high_str = str(self._high_skip_ctrl.text())
        if len(high_str.strip())>0:
            high = int(high_str)
        return low, high

    def update_skipped(self):
        """
            Update skipped points for all cross-sections
        """
        low, high = self.get_skipped()
        xmin = None
        xmax = None

        # Get info from the OFF OFF cross-section
        if self._data[ReflData.OFF_OFF] is not None:
            self._data[ReflData.OFF_OFF].set_skipped_points(low, high)
            xmin, xmax = self._data[ReflData.OFF_OFF].get_skipped_range()
            self._data[ReflData.OFF_OFF].set_range(xmin, xmax)
        else:
            print "No Off-Off data to process"
            return xmin, xmax

        for polarization in [ReflData.ON_OFF,
                             ReflData.OFF_ON,
                             ReflData.ON_ON]:
            if self._data[polarization] is not None:
                self._data[polarization].set_skipped_points(low, high)
                self._data[polarization].set_range(xmin, xmax)

        return xmin, xmax

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
        self.connect(self._content.pick_unity_range_btn, QtCore.SIGNAL("clicked()"), self._pick_specular_ridge)
        self.connect(self._content.auto_scale_btn, QtCore.SIGNAL("clicked()"), self._apply)
        self.connect(self._content.save_btn, QtCore.SIGNAL("clicked()"), self._set_unity_scale)
        self._content.min_q_unity_edit.setText("0.00")
        self._content.max_q_unity_edit.setText("0.01")
        self._content.max_q_unity_edit.setValidator(QtGui.QDoubleValidator(self._content.max_q_unity_edit))

        # Email options
        self._content.send_email_chk.setChecked(False)
        self._email_options_changed()
        self.connect(self._content.send_email_chk, QtCore.SIGNAL("clicked()"), self._email_options_changed)
        self.connect(self._content.send_btn, QtCore.SIGNAL("clicked()"), self._email_result)
        import getpass
        sender_email = getpass.getuser()+"@ornl.gov"
        self._content.from_email_edit.setText(sender_email)

        if self._settings.instrument_name == "REFL":
            self._content.ref_pol_label.hide()
            self._content.off_off_radio.hide()
            self._content.on_off_radio.hide()
            self._content.off_on_radio.hide()
            self._content.on_on_radio.hide()
        else:
            self.radio_group = QtGui.QButtonGroup()
            self.radio_group.addButton(self._content.off_off_radio)
            self.radio_group.addButton(self._content.off_on_radio)
            self.radio_group.addButton(self._content.on_off_radio)
            self.radio_group.addButton(self._content.on_on_radio)
            self.radio_group.setExclusive(True)

    def _set_unity_scale(self):
        """
            Set scaling factors to reference
        """
        ref= 0
        for item in self._workspace_list:
            if item.is_selected():
                ref = item.get_scale()
                break
        for item in self._workspace_list:
            item.set_scale(ref)
        self.plot_result()


    def _email_options_changed(self):
        """
            Send-email checkbox has changed states
        """
        is_checked = self._content.send_email_chk.isChecked()
        self._content.to_email_label.setEnabled(is_checked)
        self._content.to_email_edit.setEnabled(is_checked)
        self._content.from_email_label.setEnabled(is_checked)
        self._content.from_email_edit.setEnabled(is_checked)
        self._content.send_btn.setEnabled(is_checked)

    def _add_entry(self, workspace):
        entry = ReflData(workspace, parent_layout=self._content.angle_list_layout)
        entry.connect_to_scale(self.plot_result)
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
        try:
            self._scale_data_sets()
        except:
            message = "Could not scale data\n  %s" % sys.exc_value
            if self._content.scale_to_one_chk.isChecked():
                message += "\n\nCheck your Q range near the critical edge"
            QtGui.QMessageBox.warning(self,\
                "Error scaling data",\
                message)

    def _pick_specular_ridge(self):
        from LargeScaleStructures import data_stitching
        refID = 0
        if len(self._workspace_list)==0:
            return

        for i in range(len(self._workspace_list)):
            item = self._workspace_list[i]
            if item.is_selected():
                refID = i

        def call_back(xmin, xmax):
            self._content.min_q_unity_edit.setText("%-g" % xmin)
            self._content.max_q_unity_edit.setText("%-g" % xmax)

        if AnalysisDataService.doesExist(self._workspace_list[refID].name):
            data_stitching.RangeSelector.connect([self._workspace_list[refID].name], call_back)

    def _scale_data_sets(self):
        """
            Perform auto-scaling
        """
        scale_to_unity = self._content.scale_to_one_chk.isChecked()
        min_q_unity = float(self._content.min_q_unity_edit.text())
        max_q_unity = float(self._content.max_q_unity_edit.text())

        s = Stitcher()
        refID = 0

        # Get reference cross-section
        ref_pol = ReflData.OFF_OFF
        if self._content.off_on_radio.isChecked():
            ref_pol = ReflData.OFF_ON
        elif self._content.on_off_radio.isChecked():
            ref_pol = ReflData.ON_OFF
        elif self._content.on_on_radio.isChecked():
            ref_pol = ReflData.ON_ON

        for i in range(len(self._workspace_list)):
            item = self._workspace_list[i]
            data = DataSet(item.name)
            data.load(True, True)
            item.set_user_data(data)

            # Set skipped points for all cross-section
            xmin, xmax = item.update_skipped()

            if item.is_selected():
                data.set_scale(item.get_scale())
                refID = i

                if scale_to_unity:
                    scale = data.scale_to_unity(max(xmin,min_q_unity), min(xmax,max_q_unity))
                    data.set_scale(scale)
                    item.set_scale(scale)

            ref_data = item.get_user_data(ref_pol)
            if ref_data is None:
                QtGui.QMessageBox.warning(self,\
                    "Invalid choice of reference cross-section",\
                    "The selected cross-section is empty, please select another one")
                return
            s.append(ref_data)

        if s.size()==0:
            Logger("Stitcher").notice("No data to scale")
            return

        s.set_reference(refID)
        s.compute()

        for item in self._workspace_list:
            data = item.get_user_data(ref_pol)
            xmin, xmax = item.get_common_range()
            data.apply_scale(xmin=xmin, xmax=xmax)
            scale = data.get_scale()
            item.set_scale(scale)

        self._stitcher = s

        self.plot_result()

    def plot_result(self):
        """
            Plot the scaled data sets
        """
        pol_dict = {"Off Off" : ReflData.OFF_OFF,
                    "On Off" : ReflData.ON_OFF,
                    "Off On" : ReflData.OFF_ON,
                    "On On"  : ReflData.ON_ON }

        for pol in pol_dict.keys():
            s = Stitcher()
            ws_list = []
            for item in self._workspace_list:
                d = item.get_user_data( pol_dict[pol] )
                if d is not None:
                    ws_list.append(d.get_scaled_ws())
                    s.append(d)

            if len(ws_list)>0:
                combined_ws = "ref_%s" % pol.replace(" ", "_")
                if self._settings.instrument_name == "REFL":
                    combined_ws = "ref_combined"
                s.get_scaled_data(combined_ws)

                plot_name = '%s: %s' % (self._graph, pol)
                g = mantidplot.graph(plot_name)
                if g is not None:
                    continue
                g = mantidplot.plotSpectrum(ws_list, [0], True)
                g.setName(plot_name)
                l=g.activeLayer()
                l.logYlinX()
                if self._settings.instrument_name == "REFL":
                    l.setTitle("Reflectivity")
                else:
                    l.setTitle("Polarization state: %s" % pol)

    def _email_result(self):
        self._save_result(send_email=True)

    def _save_result(self, send_email=False):
        """
            Save the scaled output in one combined I(Q) file
        """
        if self._stitcher is not None:
            if send_email:
                fname = os.path.join(os.path.expanduser("~"),"reflectivity.txt")
            else:
                if self._output_dir is None or not os.path.isdir(self._output_dir):
                    self._output_dir = os.path.expanduser("~")
                fname_qstr = QtGui.QFileDialog.getSaveFileName(self, "Save combined reflectivity",
                                                               self._output_dir,
                                                               "Data Files (*.txt)")
                fname = str(QtCore.QFileInfo(fname_qstr).filePath())
            file_list = []
            if len(fname)>0:
                if self._settings.instrument_name == "REFL":
                    self._stitcher.save_combined(fname, as_canSAS=False)
                    file_list.append(fname)
                else:
                    pol_list = ["Off_Off", "On_Off",\
                               "Off_On", "On_On"]
                    for pol in pol_list:
                        try:
                            if AnalysisDataService.doesExist('ref_'+pol):
                                root, ext = os.path.splitext(os.path.basename(fname))
                                outdir, filename = os.path.split(fname)
                                outname = "%s_%s.txt" % (root, pol)

                                file_path = os.path.join(outdir, outname)
                                SaveAscii(Filename=file_path,
                                          InputWorkspace="ref_"+pol,
                                          Separator="Space")
                                file_list.append(file_path)
                        except:
                            Logger("Stitcher").notice("Could not save polarization %s" % pol)
            if send_email:
                self._email_data(file_list)

    def _email_data(self, file_list):
        """
            Send a list of file by email
            @param file_list: list of files to send
        """
        if len(file_list)==0:
            return

        import smtplib
        from email.mime.text import MIMEText
        from email.mime.multipart import MIMEMultipart
        sender_email = str(self._content.from_email_edit.text())
        recv_email = str(self._content.to_email_edit.text())

        msg = MIMEMultipart()
        msg["Subject"] = "[Mantid] Reduced data"
        msg["From"] = sender_email
        msg["To"] = recv_email
        msg.preamble = "Reduced data"

        body = MIMEText("The attached reflectivity files were sent by MantidPlot.")
        msg.attach(body)

        for file_path in file_list:
            output_file = open(file_path, 'r')
            file_name = os.path.basename(file_path)
            att = MIMEText(output_file.read())
            att.add_header('Content-Disposition', 'attachment', filename=file_name)
            output_file.close()
            msg.attach(att)

        s = smtplib.SMTP('localhost')
        s.sendmail(recv_email, recv_email, msg.as_string())
        s.quit()

    def set_state(self, state):
        """
            Update the catalog according to the new data path
        """
        #Remove current entries
        for item in self._workspace_list:
            item.delete()

        self._workspace_list = []

        # Refresh combo boxes
        if self._settings.instrument_name == "REFL":
            _tmp_workspace_list = []
            for item in AnalysisDataService.getObjectNames():
                #retrieve workspaces of interest
                if item.startswith("reflectivity") and item.endswith("ts"):
                    _tmp_workspace_list.append(item)
            #sort the items by time stamp
            _list_name = []
            _list_ts = []
            if _tmp_workspace_list != []:
                for _item in _tmp_workspace_list:
                    (_name,_ts) = _item.split('_#')
                    _list_name.append(_name)
                    _list_ts.append(_ts)

                _name_ts = zip(_list_ts, _list_name)
                _name_ts.sort()
                _ts_sorted, _name_sorted = zip(*_name_ts)

                for item in _name_sorted:
                    self._add_entry(item)

        else: #REF_M
            for item in AnalysisDataService.getObjectNames():
                if item.startswith("reflectivity") and not item.endswith("scaled") and item.find('On_Off')<0 and item.find('Off_On')<0  and item.find('On_On')<0:
                    self._add_entry(item)

        if len(self._workspace_list)>0:
            self._workspace_list[0].select()
            self._apply()

    def get_state(self):
        """
            Return dummy state
        """
        return StitcherState()
