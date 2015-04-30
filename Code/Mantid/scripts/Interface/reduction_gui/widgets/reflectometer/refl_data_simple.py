#pylint: disable=invalid-name
from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import time
import sys
from functools import partial
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets as REFLDataSets
from reduction_gui.reduction.reflectometer.refm_data_script import DataSets as REFMDataSets
from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_data_refl_simple

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from mantid.simpleapi import *
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
    instrument_name = 'REF_L'
    short_name = 'REFL'
    peak_pixel_range = []
    background_pixel_range = []
    _run_number_first_edit = None

    def __init__(self, parent=None, state=None, settings=None, name="REFL", data_proxy=None):
        super(DataReflWidget, self).__init__(parent, state, settings, data_proxy=data_proxy)

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_data_refl_simple.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self.short_name = name
        self._settings.instrument_name = name

        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        self._detector_distance = 1.0
        self._sangle_parameter = 0.0

        if state is not None:
            self.set_state(state)
        else:
            if self.instrument_name == "REF_L":
                self.set_state(DataSeries(data_class=REFLDataSets))
            else:
                self.set_state(DataSeries(data_class=REFMDataSets))

    def initialize_content(self):
        self._summary.edited_warning_label.hide()

        # Validators
        self._summary.data_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_from_pixel))
        self._summary.data_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_to_pixel))
        self._summary.data_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel1))
        self._summary.data_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel1))
        self._summary.data_from_tof.setValidator(QtGui.QIntValidator(self._summary.data_from_tof))
        self._summary.data_to_tof.setValidator(QtGui.QIntValidator(self._summary.data_to_tof))
        self._summary.dq0.setValidator(QtGui.QDoubleValidator(self._summary.dq0))
        self._summary.dq_over_q.setValidator(QtGui.QDoubleValidator(self._summary.dq_over_q))

        self._summary.x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_min_edit))
        self._summary.x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_max_edit))
        self._summary.norm_x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_min_edit))
        self._summary.norm_x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_max_edit))

        self._summary.log_scale_chk.setChecked(True)
        self._summary.q_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.q_min_edit))

        if self.instrument_name == "REF_L":
            self._summary.q_step_edit.setValidator(QtGui.QDoubleValidator(self._summary.q_step_edit))
        else:
            self._summary.q_step_edit.setValidator(QtGui.QIntValidator(self._summary.q_step_edit))

        self._summary.angle_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_edit))
        self._summary.center_pix_edit.setValidator(QtGui.QDoubleValidator(self._summary.center_pix_edit))

        self._summary.angle_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_edit))
        self._summary.angle_offset_error_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_error_edit))

        self._summary.norm_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_from_pixel))
        self._summary.norm_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_to_pixel))
        self._summary.norm_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_from_pixel1))
        self._summary.norm_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_to_pixel1))

        self._summary.det_angle_edit.setValidator(QtGui.QDoubleValidator(self._summary.det_angle_edit))
        self._summary.det_angle_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.det_angle_offset_edit))
        self._summary.direct_pixel_edit.setValidator(QtGui.QDoubleValidator(self._summary.direct_pixel_edit))

        # Event connections
        self.connect(self._summary.data_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._data_low_res_clicked)
        self.connect(self._summary.norm_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_low_res_clicked)
        self.connect(self._summary.norm_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_clicked)
        self.connect(self._summary.norm_background_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_background_clicked)
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.tof_range_switch, QtCore.SIGNAL("clicked(bool)"), self._tof_range_clicked)
        self.connect(self._summary.geometry_correction_switch, QtCore.SIGNAL("clicked(bool)"), self._geometry_correction_clicked)
        self.connect(self._summary.plot_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y)
        self.connect(self._summary.plot_count_vs_x_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_x)
        self.connect(self._summary.plot_count_vs_y_bck_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y_bck)
        self.connect(self._summary.norm_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_y)
        self.connect(self._summary.norm_count_vs_x_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_x)
        self.connect(self._summary.norm_count_vs_y_bck_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_y_bck)
        self.connect(self._summary.plot_tof_btn, QtCore.SIGNAL("clicked()"), self._plot_tof)
        self.connect(self._summary.add_dataset_btn, QtCore.SIGNAL("clicked()"), self._add_data)
        self.connect(self._summary.angle_list, QtCore.SIGNAL("itemSelectionChanged()"), self._angle_changed)
        self.connect(self._summary.remove_btn, QtCore.SIGNAL("clicked()"), self._remove_item)
        self.connect(self._summary.fourth_column_switch, QtCore.SIGNAL("clicked(bool)"), self._fourth_column_clicked)

        # Catch edited controls
        call_back = partial(self._edit_event, ctrl=self._summary.data_peak_from_pixel)
        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_peak_to_pixel)
        self.connect(self._summary.data_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_background_from_pixel1)
        self.connect(self._summary.data_background_from_pixel1, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_background_to_pixel1)
        self.connect(self._summary.data_background_to_pixel1, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_from_tof)
        self.connect(self._summary.data_from_tof, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_to_tof)
        self.connect(self._summary.data_to_tof, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.x_min_edit)
        self.connect(self._summary.x_min_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.x_max_edit)
        self.connect(self._summary.x_max_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_x_min_edit)
        self.connect(self._summary.norm_x_min_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_x_max_edit)
        self.connect(self._summary.norm_x_max_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        #call_back = partial(self._edit_event, ctrl=self._summary.q_min_edit)
        #self.connect(self._summary.q_min_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        #call_back = partial(self._edit_event, ctrl=self._summary.q_step_edit)
        #self.connect(self._summary.q_step_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.angle_edit)
        self.connect(self._summary.angle_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.center_pix_edit)
        self.connect(self._summary.center_pix_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.angle_offset_edit)
        self.connect(self._summary.angle_offset_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.angle_offset_error_edit)
        self.connect(self._summary.angle_offset_error_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_peak_from_pixel)
        self.connect(self._summary.norm_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_peak_to_pixel)
        self.connect(self._summary.norm_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_background_from_pixel1)
        self.connect(self._summary.norm_background_from_pixel1, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_background_to_pixel1)
        self.connect(self._summary.norm_background_to_pixel1, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.norm_run_number_edit)
        self.connect(self._summary.norm_run_number_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_run_number_edit)
        self.connect(self._summary.data_run_number_edit, QtCore.SIGNAL("textChanged(QString)"), self._run_number_changed)
        self._run_number_first_edit = True
        #call_back = partial(self._edit_event, ctrl=self._summary.log_scale_chk)
        #self.connect(self._summary.log_scale_chk, QtCore.SIGNAL("clicked()"), call_back)

        call_back = partial(self._edit_event, ctrl=self._summary.det_angle_edit)
        self.connect(self._summary.det_angle_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.det_angle_offset_edit)
        self.connect(self._summary.det_angle_offset_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.direct_pixel_edit)
        self.connect(self._summary.direct_pixel_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)

        self.connect(self._summary.det_angle_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.det_angle_offset_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.direct_pixel_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.center_pix_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)

        call_back = partial(self._edit_event, ctrl=self._summary.direct_pixel_check)
        self.connect(self._summary.direct_pixel_check, QtCore.SIGNAL("clicked()"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.det_angle_check)
        self.connect(self._summary.det_angle_check, QtCore.SIGNAL("clicked()"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.det_angle_offset_check)
        self.connect(self._summary.det_angle_offset_check, QtCore.SIGNAL("clicked()"), call_back)

        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), self._data_peak_range_changed)
        self.connect(self._summary.data_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), self._data_peak_range_changed)

        # Output directory
        self._summary.outdir_edit.setText(os.path.expanduser('~'))
        self.connect(self._summary.outdir_browse_button, QtCore.SIGNAL("clicked()"), self._output_dir_browse)

        # Set up the automated reduction options
        self._summary.auto_reduce_check.setChecked(False)
        self._auto_reduce(False)
        self.connect(self._summary.auto_reduce_check, QtCore.SIGNAL("clicked(bool)"), self._auto_reduce)
        self.connect(self._summary.auto_reduce_btn, QtCore.SIGNAL("clicked()"), self._create_auto_reduce_template)

        # Set up the scattering angle options
        self._summary.angle_radio.setChecked(True)
        self._scattering_angle_changed()
        self.connect(self._summary.angle_radio, QtCore.SIGNAL("clicked()"), self._scattering_angle_changed)
        self.connect(self._summary.center_pix_radio, QtCore.SIGNAL("clicked()"), self._scattering_angle_changed)
        self.connect(self._summary.det_angle_check, QtCore.SIGNAL("clicked()"), self._det_angle_chk_changed)
        self.connect(self._summary.det_angle_offset_check, QtCore.SIGNAL("clicked()"), self._det_angle_offset_chk_changed)
        self.connect(self._summary.direct_pixel_check, QtCore.SIGNAL("clicked()"), self._direct_pixel_chk_changed)
        self._det_angle_chk_changed()
        self._det_angle_offset_chk_changed()
        self._direct_pixel_chk_changed()

        # Get instrument selection
        if self.short_name == "REFL":
            self._summary.refl_radio.setChecked(True)
        else:
            self._summary.refm_radio.setChecked(True)
        self.connect(self._summary.refl_radio, QtCore.SIGNAL("clicked()"), self._ref_instrument_selected)
        self.connect(self._summary.refm_radio, QtCore.SIGNAL("clicked()"), self._ref_instrument_selected)
        self._ref_instrument_selected()
        self._summary.instrument_group_box.hide()
        self._summary.waiting_label.hide()

        # If we do not have access to /SNS, don't display the automated reduction options
        if not self._settings.debug and not os.path.isdir("/SNS/%s" % self.instrument_name):
            self._summary.auto_reduce_check.hide()

    def _my_test(self):
        print "I'm in my test"

    def _output_dir_browse(self):
        output_dir = QtGui.QFileDialog.getExistingDirectory(self, "Output Directory - Choose a directory",
                                                            os.path.expanduser('~'),
                                                            QtGui.QFileDialog.ShowDirsOnly
                                                            | QtGui.QFileDialog.DontResolveSymlinks)
        if output_dir:
            self._summary.outdir_edit.setText(output_dir)

    def _run_number_changed(self):
        self._edit_event(ctrl=self._summary.data_run_number_edit)

    def _edit_event(self, text=None, ctrl=None):
        self._summary.edited_warning_label.show()
        util.set_edited(ctrl,True)

    def _reset_warnings(self):
        self._summary.edited_warning_label.hide()
        util.set_edited(self._summary.data_peak_from_pixel, False)
        util.set_edited(self._summary.data_peak_to_pixel, False)
        util.set_edited(self._summary.data_background_from_pixel1, False)
        util.set_edited(self._summary.data_background_to_pixel1, False)
        util.set_edited(self._summary.data_from_tof, False)
        util.set_edited(self._summary.data_to_tof, False)
        util.set_edited(self._summary.x_min_edit, False)
        util.set_edited(self._summary.x_max_edit, False)
        util.set_edited(self._summary.norm_x_min_edit, False)
        util.set_edited(self._summary.norm_x_max_edit, False)
        util.set_edited(self._summary.q_min_edit, False)
        util.set_edited(self._summary.q_step_edit, False)
        util.set_edited(self._summary.angle_edit, False)
        util.set_edited(self._summary.center_pix_edit, False)
        util.set_edited(self._summary.angle_offset_edit, False)
        util.set_edited(self._summary.angle_offset_error_edit, False)
        util.set_edited(self._summary.norm_peak_from_pixel, False)
        util.set_edited(self._summary.norm_peak_to_pixel, False)
        util.set_edited(self._summary.norm_background_from_pixel1, False)
        util.set_edited(self._summary.norm_background_to_pixel1, False)
        util.set_edited(self._summary.norm_run_number_edit, False)
        util.set_edited(self._summary.data_run_number_edit, False)
        util.set_edited(self._summary.log_scale_chk, False)
        util.set_edited(self._summary.angle_radio,False)
        util.set_edited(self._summary.center_pix_radio,False)
        util.set_edited(self._summary.data_background_switch, False)
        util.set_edited(self._summary.norm_background_switch, False)
        util.set_edited(self._summary.data_low_res_range_switch, False)
        util.set_edited(self._summary.norm_low_res_range_switch, False)
        util.set_edited(self._summary.norm_switch, False)
        util.set_edited(self._summary.tof_range_switch, False)
        util.set_edited(self._summary.geometry_correction_switch, False)
        util.set_edited(self._summary.q_min_edit, False)
        util.set_edited(self._summary.q_step_edit, False)
        util.set_edited(self._summary.det_angle_check, False)
        util.set_edited(self._summary.det_angle_edit, False)
        util.set_edited(self._summary.det_angle_offset_check, False)
        util.set_edited(self._summary.det_angle_offset_edit, False)
        util.set_edited(self._summary.direct_pixel_check, False)
        util.set_edited(self._summary.direct_pixel_edit, False)

    def _det_angle_offset_chk_changed(self):
        is_checked = self._summary.det_angle_offset_check.isChecked()
        self._summary.det_angle_offset_edit.setEnabled(is_checked)

    def _det_angle_chk_changed(self):
        is_checked = self._summary.det_angle_check.isChecked()
        self._summary.det_angle_edit.setEnabled(is_checked)

    def _direct_pixel_chk_changed(self):
        is_checked = self._summary.direct_pixel_check.isChecked()
        self._summary.direct_pixel_edit.setEnabled(is_checked)

    def _scattering_angle_changed(self):
        if self._summary.center_pix_radio.isChecked():
            self._summary.angle_edit.setEnabled(False)
            self._summary.center_pix_edit.setEnabled(True)
            self._summary.det_angle_check.setEnabled(True)
            self._det_angle_chk_changed()
            self._summary.det_angle_offset_check.setEnabled(True)
            self._det_angle_offset_chk_changed()
            self._summary.direct_pixel_check.setEnabled(True)
            self._direct_pixel_chk_changed()
            self._summary.det_angle_unit_label.setEnabled(True)
            self._summary.det_angle_offset_unit_label.setEnabled(True)
        else:
            self._summary.angle_edit.setEnabled(True)
            self._summary.center_pix_edit.setEnabled(False)
            self._summary.det_angle_check.setEnabled(False)
            self._summary.det_angle_edit.setEnabled(False)
            self._summary.det_angle_offset_check.setEnabled(False)
            self._summary.det_angle_offset_edit.setEnabled(False)
            self._summary.direct_pixel_check.setEnabled(False)
            self._summary.direct_pixel_edit.setEnabled(False)
            self._summary.det_angle_unit_label.setEnabled(False)
            self._summary.det_angle_offset_unit_label.setEnabled(False)

        util.set_edited(self._summary.angle_radio,True)
        util.set_edited(self._summary.center_pix_radio,True)

    def _data_peak_range_changed(self):
        """
            Update the direct pixel value when the data peak
            range changes
        """
        if not self._summary.center_pix_radio.isChecked():
            min_pix = float(self._summary.data_peak_from_pixel.text())
            max_pix = float(self._summary.data_peak_to_pixel.text())
            dir_pix = (max_pix+min_pix)/2.0
            dir_pix_str = "%4.4g" % dir_pix
            self._summary.center_pix_edit.setText(dir_pix_str.strip())

    def _update_scattering_angle(self):
        if not self._summary.angle_radio.isChecked():
            dangle = util._check_and_get_float_line_edit(self._summary.det_angle_edit)
            dangle0 = util._check_and_get_float_line_edit(self._summary.det_angle_offset_edit)
            direct_beam_pix = util._check_and_get_float_line_edit(self._summary.direct_pixel_edit)
            ref_pix = util._check_and_get_float_line_edit(self._summary.center_pix_edit)
            PIXEL_SIZE = 0.0007 # m

            delta = (dangle-dangle0)*math.pi/180.0/2.0\
                + ((direct_beam_pix-ref_pix)*PIXEL_SIZE)/ (2.0*self._detector_distance)

            scattering_angle = delta*180.0/math.pi
            scattering_angle_str = "%4.3g" % scattering_angle
            self._summary.angle_edit.setText(scattering_angle_str.strip())

    def _ref_instrument_selected(self):
        if self._summary.refl_radio.isChecked():
            self.instrument_name = "REF_L"
            self._summary.center_pix_radio.hide()
            self._summary.center_pix_edit.hide()
            self._summary.angle_radio.hide()
            self._summary.angle_edit.hide()
            self._summary.angle_unit_label.hide()
            self._summary.angle_offset_label.show()
            self._summary.angle_offset_edit.show()
            self._summary.angle_offset_pm_label.show()
            self._summary.angle_offset_error_edit.show()
            self._summary.angle_offset_unit_label.show()
            self._summary.det_angle_offset_check.hide()
            self._summary.det_angle_offset_edit.hide()
            self._summary.det_angle_offset_unit_label.hide()
            self._summary.det_angle_check.hide()
            self._summary.det_angle_edit.hide()
            self._summary.det_angle_unit_label.hide()
            self._summary.direct_pixel_check.hide()
            self._summary.direct_pixel_edit.hide()
            self._summary.q_bins_label.hide()
            self._summary.ref_pix_estimate.hide()

            # Output directory
            self._summary.outdir_label.hide()
            self._summary.outdir_edit.hide()
            self._summary.outdir_browse_button.hide()
        else:
            self.instrument_name = "REF_M"
            self._summary.center_pix_radio.show()
            self._summary.center_pix_edit.show()
            self._summary.angle_radio.show()
            self._summary.angle_edit.show()
            self._summary.angle_unit_label.show()
            self._summary.angle_offset_label.hide()
            self._summary.angle_offset_edit.hide()
            self._summary.angle_offset_pm_label.hide()
            self._summary.angle_offset_error_edit.hide()
            self._summary.angle_offset_unit_label.hide()
            self._summary.q_step_label.hide()
            self._summary.q_step_unit_label.hide()
            self._summary.q_min_edit.hide()
            self._summary.q_min_label.hide()
            self._summary.q_min_unit_label.hide()

            #TODO: allow log binning
            self._summary.log_scale_chk.hide()

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
        script += "import mantid\n"
        script += "from mantid.simpleapi import *\n"
        script += "from mantid.kernel import ConfigService\n"

        content += "eventFileAbs=sys.argv[1]\n"
        content += "outputDir=sys.argv[2]\n\n"

        content += "eventFile = os.path.split(eventFileAbs)[-1]\n"
        content += "nexusDir = eventFileAbs.replace(eventFile, '')\n"
        content += "runNumber = eventFile.split('_')[2]\n"
        content += "ConfigService.Instance().appendDataSearchDir(nexusDir)\n\n"

        # Place holder for reduction script
        content += "\n"
        content += "# Place holder for python script\n"
        content += "file_path = os.path.join(outputDir, '%s_'+runNumber+'.py')\n" % self.instrument_name
        content += "f=open(file_path,'w')\n"
        content += "f.write(\"runNumber=\%s \% runNumber\\n\")\n"
        content += "f.write(\"\"\"%s\"\"\")\n" % reduce_script
        content += "f.close()\n\n"

        # Reduction option to load into Mantid
        xml_str = "<Reduction>\n"
        xml_str += "  <instrument_name>%s</instrument_name>\n" % self.short_name
        xml_str += "  <timestamp>%s</timestamp>\n" % time.ctime()
        xml_str += "  <python_version>%s</python_version>\n" % sys.version
        if IS_IN_MANTIDPLOT:
            xml_str += "  <mantid_version>%s</mantid_version>\n" % mantid_build_version()
        xml_str += m.to_xml()
        xml_str += "</Reduction>\n"

        content += "# Reduction options for loading into Mantid\n"
        content += "file_path = os.path.join(outputDir, '%s_'+runNumber+'.xml')\n" % self.instrument_name
        content += "f=open(file_path,'w')\n"
        content += "f.write(\"\"\"%s\"\"\")\n" % xml_str
        content += "f.close()\n"

        content += reduce_script

        content += "\n"
        content += "for item in mtd.keys():\n"
        content += "    if item.startswith('reflectivity_'):\n"
        content += "        file_name = item+'.txt'\n"
        content += "        file_path = os.path.join(outputDir,file_name)\n"
        content += "        SaveAscii(Filename=file_path,\n"
        content += "          InputWorkspace=item,\n"
        content += "          Separator='Tab',\n"
        content += "          CommentIndicator='# ')\n"

        home_dir = os.path.expanduser('~')
        f=open(os.path.join(home_dir,"reduce_%s.py" % self.instrument_name),'w')
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
            message += os.path.join(home_dir,"reduce_%s.py" % self.instrument_name)
            message += "\n\nTry copying it by hand in %s\n" % sns_path
            QtGui.QMessageBox.warning(self, "Error saving automated reduction script", message)

        sns_path = "/SNS/%s/shared/autoreduce" % self.instrument_name
        if os.path.isdir(sns_path):
            if os.access(sns_path, os.W_OK):
                file_path = os.path.join(sns_path,"reduce_%s.py" % self.instrument_name)
                if os.path.isfile(file_path) and not os.access(file_path, os.W_OK):
                    _report_error("You do not have permissions to overwrite %s." % file_path)
                    return
                try:
                    f = open(file_path,'w')
                    f.write(content)
                    f.close()
                    QtGui.QMessageBox.information(self, "Automated reduction script saved",\
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
        if self._summary.angle_list.count()==0:
            return
        self._summary.angle_list.setEnabled(False)
        self._summary.remove_btn.setEnabled(False)
        row = self._summary.angle_list.currentRow()
        if row>=0:
            self._summary.angle_list.takeItem(row)
        self._summary.angle_list.setEnabled(True)
        self._summary.remove_btn.setEnabled(True)

    def is_running(self, is_running):
        """
            Enable/disable controls depending on whether a reduction is running or not
            @param is_running: True if a reduction is running
        """
        super(DataReflWidget, self).is_running(is_running)
        self.setEnabled(not is_running)
        self._summary.plot_count_vs_y_btn.setEnabled(not is_running)
        self._summary.plot_count_vs_y_bck_btn.setEnabled(not is_running)
        self._summary.plot_count_vs_x_btn.setEnabled(not is_running)
        self._summary.norm_count_vs_y_btn.setEnabled(not is_running)
        self._summary.norm_count_vs_y_bck_btn.setEnabled(not is_running)
        self._summary.norm_count_vs_x_btn.setEnabled(not is_running)
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
        self._edit_event(None, self._summary.data_background_switch)

    def _norm_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.norm_background_from_pixel1.setEnabled(is_checked)
        self._summary.norm_background_from_pixel1_label.setEnabled(is_checked)
        self._summary.norm_background_to_pixel1.setEnabled(is_checked)
        self._summary.norm_background_to_pixel1_label.setEnabled(is_checked)
        self._edit_event(None, self._summary.norm_background_switch)

    def _data_low_res_clicked(self, is_checked):
        """
            This is reached when the user clicks the Data Low-Res axis range switch
        """
        self._summary.data_low_res_from_label.setEnabled(is_checked)
        self._summary.x_min_edit.setEnabled(is_checked)
        self._summary.data_low_res_to_label.setEnabled(is_checked)
        self._summary.x_max_edit.setEnabled(is_checked)
        self._edit_event(None, self._summary.data_low_res_range_switch)

    def _norm_low_res_clicked(self, is_checked):
        """
            This is reached when the user clicks the Data Low-Res axis range switch
        """
        self._summary.norm_low_res_from_label.setEnabled(is_checked)
        self._summary.norm_x_min_edit.setEnabled(is_checked)
        self._summary.norm_low_res_to_label.setEnabled(is_checked)
        self._summary.norm_x_max_edit.setEnabled(is_checked)
        self._edit_event(None, self._summary.norm_low_res_range_switch)

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
        if not is_checked:
            self._norm_background_clicked(False)
        else:
            NormBackFlag = self._summary.norm_background_switch.isChecked()
            self._norm_background_clicked(NormBackFlag)

        self._summary.norm_low_res_range_switch.setEnabled(is_checked)
        if not is_checked:
            self._norm_low_res_clicked(False)
        else:
            LowResFlag = self._summary.norm_low_res_range_switch.isChecked()
            self._norm_low_res_clicked(LowResFlag)

        self._edit_event(None, self._summary.norm_switch)

    def _fourth_column_clicked(self, is_checked):
        """
            This is reached by the 4th column switch
        """
        self._summary.dq0_label.setEnabled(is_checked)
        self._summary.dq0.setEnabled(is_checked)
        self._summary.dq0_unit.setEnabled(is_checked)
        self._summary.dq_over_q_label.setEnabled(is_checked)
        self._summary.dq_over_q.setEnabled(is_checked)

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
        #self._summary.plot_tof_btn.setEnabled(is_checked)
        self._edit_event(None, self._summary.tof_range_switch)

    def _geometry_correction_clicked(self, is_checked):
        """
        this is reached by the geometry correction switch
        """
        pass

    def _plot_count_vs_y(self, is_peak=True):
        """
            Plot counts as a function of high-resolution pixels
            and select peak range
            For REFM, this is X
            For REFL, this is Y
        """
        min, max = self._integrated_plot(True,
                                         self._summary.data_run_number_edit,
                                         self._summary.data_peak_from_pixel,
                                         self._summary.data_peak_to_pixel)
        self._summary.data_peak_from_pixel_estimate.setText(str(int(math.ceil(min))))
        self._summary.data_peak_to_pixel_estimate.setText(str(int(math.ceil(max))))
        self._summary.ref_pix_estimate.setText("%4.1f" % ((max+min)/2.0))
        util.set_tiny(self._summary.data_peak_from_pixel_estimate)
        util.set_tiny(self._summary.data_peak_to_pixel_estimate)
        util.set_tiny(self._summary.ref_pix_estimate)

    def _plot_count_vs_y_bck(self):
        """
            Plot counts as a function of high-resolution pixels
            and select background range
            For REFM, this is X
            For REFL, this is Y
        """
        self._integrated_plot(True,
                              self._summary.data_run_number_edit,
                              self._summary.data_background_from_pixel1,
                              self._summary.data_background_to_pixel1)

    def _plot_count_vs_x(self):
        """
            Plot counts as a function of low-resolution pixels
            For REFM, this is Y
            For REFL, this is X
        """
        min, max = self._integrated_plot(False,
                                         self._summary.data_run_number_edit,
                                         self._summary.x_min_edit,
                                         self._summary.x_max_edit)
        self._summary.x_min_estimate.setText(str(int(math.ceil(min))))
        self._summary.x_max_estimate.setText(str(int(math.ceil(max))))
        util.set_tiny(self._summary.x_min_estimate)
        util.set_tiny(self._summary.x_max_estimate)

    def _norm_count_vs_y(self):
        min, max = self._integrated_plot(True,
                                         self._summary.norm_run_number_edit,
                                         self._summary.norm_peak_from_pixel,
                                         self._summary.norm_peak_to_pixel)
        self._summary.norm_peak_from_pixel_estimate.setText(str(int(math.ceil(min))))
        self._summary.norm_peak_to_pixel_estimate.setText(str(int(math.ceil(max))))
        util.set_tiny(self._summary.norm_peak_from_pixel_estimate)
        util.set_tiny(self._summary.norm_peak_to_pixel_estimate)

    def _norm_count_vs_y_bck(self):
        self._integrated_plot(True,
                              self._summary.norm_run_number_edit,
                              self._summary.norm_background_from_pixel1,
                              self._summary.norm_background_to_pixel1)

    def _norm_count_vs_x(self):
        min, max = self._integrated_plot(False,
                                         self._summary.norm_run_number_edit,
                                         self._summary.norm_x_min_edit,
                                         self._summary.norm_x_max_edit)
        self._summary.norm_xmin_estimate.setText(str(int(math.ceil(min))))
        self._summary.norm_xmax_estimate.setText(str(int(math.ceil(max))))
        util.set_tiny(self._summary.norm_xmin_estimate)
        util.set_tiny(self._summary.norm_xmax_estimate)

    def _integrated_plot(self, is_high_res, file_ctrl, min_ctrl, max_ctrl):
        """
            Plot counts as a function of:

            Low-resolution pixels
                For REFM, this is Y
                For REFL, this is X

            High-resolution pixels
                For REFM, this is X
                For REFL, this is Y

            @param is_high_res: True if we are plotting the high-res pixel distribution
            @param file_ctrl: control widget containing the data file name
            @param min_ctrl: control widget containing the range minimum
            @param max_ctrl: control widget containing the range maximum
        """
        if not IS_IN_MANTIDPLOT:
            return

        try:
            f = FileFinder.findRuns("%s%s" % (self.instrument_name, str(file_ctrl.text())))[0]

            range_min = int(min_ctrl.text())
            range_max = int(max_ctrl.text())

            def call_back(xmin, xmax):
                min_ctrl.setText("%-d" % int(xmin))
                max_ctrl.setText("%-d" % int(xmax))

            # For REFL, Y is high-res
            is_pixel_y = is_high_res
            # For REFM it's the other way around
            if self.short_name == "REFM":
                is_pixel_y = not is_pixel_y

            min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
                                                                      callback=call_back,
                                                                      range_min=range_min,
                                                                      range_max=range_max,
                                                                      high_res=is_high_res,
                                                                      instrument=self.short_name)
            return min, max
        except:
            pass

    def _plot_tof(self):
        if not IS_IN_MANTIDPLOT:
            return

        try:
            f = FileFinder.findRuns("%s%s" % (self.instrument_name, str(self._summary.norm_run_number_edit.text())))[0]

            range_min = int(self._summary.data_from_tof.text())
            range_max = int(self._summary.data_to_tof.text())

            def call_back(xmin, xmax):
                self._summary.data_from_tof.setText("%-d" % int(xmin))
                self._summary.data_to_tof.setText("%-d" % int(xmax))
            data_manipulation.tof_distribution(f, call_back,
                                               range_min=range_min,
                                               range_max=range_max)
        except:
            pass

    def _add_data(self):
        state = self.get_editing_state()
        in_list = False
        # Check whether it's already in the list
        run_numbers = self._summary.data_run_number_edit.text()
        list_items = self._summary.angle_list.findItems(run_numbers, QtCore.Qt.MatchFixedString)
        if len(list_items)>0:
            list_items[0].setData(QtCore.Qt.UserRole, state)
            in_list = True
        else:
            item_widget = QtGui.QListWidgetItem(run_numbers, self._summary.angle_list)
            item_widget.setData(QtCore.Qt.UserRole, state)

        # Read logs
        if not in_list and self.short_name == "REFM":
            self._read_logs()

        self._reset_warnings()

    def _read_logs(self):
        if IS_IN_MANTIDPLOT:
            # Showing the waiting message
            self._summary.waiting_label.show()
            self._summary.update()
            QtGui.QApplication.processEvents()
            QtGui.QApplication.hasPendingEvents()

            try:
                run_entry = str(self._summary.data_run_number_edit.text()).strip()
                if len(run_entry)==0 or run_entry=="0":
                    return

                logs = data_manipulation.get_logs(self.instrument_name, run_entry)
                if not self._summary.direct_pixel_check.isChecked():
                    angle_str = "%-4.3g"%logs["DIRPIX"]
                    self._summary.direct_pixel_edit.setText(angle_str.strip())
                if not self._summary.det_angle_offset_check.isChecked():
                    angle_str = "%-4.4g"%logs["DANGLE0"]
                    self._summary.det_angle_offset_edit.setText(angle_str.strip())
                if not self._summary.det_angle_check.isChecked():
                    angle_str = "%-4.4g"%logs["DANGLE"]
                    self._summary.det_angle_edit.setText(angle_str.strip())
                if not self._summary.angle_radio.isChecked():
                    angle_str = "%-4.4g"%logs["SANGLE"]
                    self._summary.angle_edit.setText(angle_str.strip())

                self._sangle_parameter = logs["SANGLE"]
                self._detector_distance = logs["DET_DISTANCE"]
            except:
                # Could not read in the parameters, skip.
                msg = "No data set was found for run %s\n\n" % run_entry
                msg += "Make sure that your data directory was added to the "
                msg += "Mantid search directories."
                QtGui.QMessageBox.warning(self,
                                          "Unable to find data set", msg)

            self._summary.waiting_label.hide()

    def _angle_changed(self):
        if self._summary.angle_list.count()==0:
            return
        self._summary.angle_list.setEnabled(False)
        self._summary.remove_btn.setEnabled(False)
        current_item =  self._summary.angle_list.currentItem()
        if current_item is not None:
            state = current_item.data(QtCore.Qt.UserRole)
            self.set_editing_state(state)
            self._reset_warnings()
        self._summary.angle_list.setEnabled(True)
        self._summary.remove_btn.setEnabled(True)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: data object
        """
        if False and IS_IN_MANTIDPLOT:
            ws_name = "reflectivity"
            ws_list = [n for n in mtd.keys() if n.startswith(ws_name)]
            g = mantidplot.graph(ws_name)
            if g is None and len(ws_list)>0:
                g = mantidplot.plotSpectrum(ws_list,[0],True)
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
            self._summary.angle_list.setCurrentRow(0, QtGui.QItemSelectionModel.Select)

            # Common Q binning
            self._summary.q_min_edit.setText(str(state.data_sets[0].q_min))
            self._summary.log_scale_chk.setChecked(state.data_sets[0].q_step<0)
            if isinstance(state.data_sets[0], REFMDataSets):
                self._summary.q_step_edit.setText(str(state.data_sets[0].q_bins))
            else:
                self._summary.q_step_edit.setText(str(math.fabs(state.data_sets[0].q_step)))

            # Common angle offset
            if hasattr(state.data_sets[0], "angle_offset"):
                self._summary.angle_offset_edit.setText(str(state.data_sets[0].angle_offset))
                self._summary.angle_offset_error_edit.setText(str(state.data_sets[0].angle_offset_error))

        self._reset_warnings()

    def set_editing_state(self, state):

        #Peak from/to pixels
        self._summary.data_peak_from_pixel.setText(str(state.DataPeakPixels[0]))
        self._summary.data_peak_to_pixel.setText(str(state.DataPeakPixels[1]))

        #data low resolution range
        self._summary.data_low_res_range_switch.setChecked(state.data_x_range_flag)
        self._summary.x_min_edit.setText(str(state.data_x_range[0]))
        self._summary.x_max_edit.setText(str(state.data_x_range[1]))
        self._data_low_res_clicked(state.data_x_range_flag)

        #norm low resolution range
        self._summary.norm_low_res_range_switch.setChecked(state.norm_x_range_flag)
        self._summary.norm_x_min_edit.setText(str(state.norm_x_range[0]))
        self._summary.norm_x_max_edit.setText(str(state.norm_x_range[1]))
        self._norm_low_res_clicked(state.data_x_range_flag)

        #Background flag
        self._summary.data_background_switch.setChecked(state.DataBackgroundFlag)
        self._data_background_clicked(state.DataBackgroundFlag)

        #Background from/to pixels
        self._summary.data_background_from_pixel1.setText(str(state.DataBackgroundRoi[0]))
        self._summary.data_background_to_pixel1.setText(str(state.DataBackgroundRoi[1]))

        #from TOF and to TOF
        self._summary.data_from_tof.setText(str(int(state.DataTofRange[0])))
        self._summary.data_to_tof.setText(str(int(state.DataTofRange[1])))
        self._summary.tof_range_switch.setChecked(state.TofRangeFlag)
        self._tof_range_clicked(state.TofRangeFlag)

        if hasattr(state, "set_detector_angle"):
            self._summary.det_angle_check.setChecked(state.set_detector_angle)
            if state.set_detector_angle:
                self._summary.det_angle_edit.setText(str(state.detector_angle).strip())
            self._summary.det_angle_offset_check.setChecked(state.set_detector_angle_offset)
            if state.set_detector_angle_offset:
                self._summary.det_angle_offset_edit.setText(str(state.detector_angle_offset).strip())
            self._summary.direct_pixel_check.setChecked(state.set_direct_pixel)
            if state.set_direct_pixel:
                self._summary.direct_pixel_edit.setText(str(state.direct_pixel).strip())
            self._det_angle_chk_changed()
            self._det_angle_offset_chk_changed()
            self._direct_pixel_chk_changed()

        # Normalization options
        self._summary.norm_run_number_edit.setText(str(state.norm_file))
        self._summary.norm_peak_from_pixel.setText(str(state.NormPeakPixels[0]))
        self._summary.norm_peak_to_pixel.setText(str(state.NormPeakPixels[1]))

        self._summary.norm_background_switch.setChecked(state.NormBackgroundFlag)
        self._norm_background_clicked(state.NormBackgroundFlag)

        self._summary.norm_background_from_pixel1.setText(str(state.NormBackgroundRoi[0]))
        self._summary.norm_background_to_pixel1.setText(str(state.NormBackgroundRoi[1]))

        #normalization flag
        self._summary.norm_switch.setChecked(state.NormFlag)
        self._norm_clicked(state.NormFlag)

        # Q binning
        self._summary.q_min_edit.setText(str(state.q_min))
        self._summary.log_scale_chk.setChecked(state.q_step<0)
        if isinstance(state, REFMDataSets):
            self._summary.q_step_edit.setText(str(state.q_bins))
        else:
            self._summary.q_step_edit.setText(str(math.fabs(state.q_step)))

        # Scattering angle
        if hasattr(state, "use_center_pixel"):
            self._summary.center_pix_edit.setText(str(state.center_pixel))
            self._summary.angle_edit.setText(str(state.theta))
            if state.use_center_pixel:
                self._summary.center_pix_radio.setChecked(True)
            else:
                self._summary.angle_radio.setChecked(True)
            self._scattering_angle_changed()

        # Output directory
        if hasattr(state, "output_dir"):
            if len(str(state.output_dir).strip())>0:
                self._summary.outdir_edit.setText(str(state.output_dir))

        self._reset_warnings()
        self._summary.data_run_number_edit.setText(str(','.join([str(i) for i in state.data_files])))

        if self.short_name == "REFM":
            self._read_logs()

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
        q_bins = 0
        if self.instrument_name == "REF_L":
            if self._summary.log_scale_chk.isChecked():
                q_step = -q_step
        else:
            q_bins = int(math.ceil(float(self._summary.q_step_edit.text())))

        # Angle offset
        if hasattr(m, "angle_offset"):
            angle_offset = float(self._summary.angle_offset_edit.text())
            angle_offset_error = float(self._summary.angle_offset_error_edit.text())

        for i in range(self._summary.angle_list.count()):
            data = self._summary.angle_list.item(i).data(QtCore.Qt.UserRole)
            # Over-write Q binning with common binning
            data.q_min = q_min
            data.q_step = q_step

            # Over-write angle offset
            if hasattr(data, "angle_offset"):
                data.angle_offset = angle_offset
                data.angle_offset_error = angle_offset_error

            if hasattr(data, "q_bins"):
                data.q_bins = q_bins
                data.q_log = self._summary.log_scale_chk.isChecked()

            if hasattr(data, "output_dir"):
                data.output_dir = self._summary.outdir_edit.text()

            state_list.append(data)
        state.data_sets = state_list

        return state

    def get_editing_state(self):
        if self.instrument_name == "REF_L":
            m = REFLDataSets()
        else:
            m = REFMDataSets()

        #Peak from/to pixels
        m.DataPeakPixels = [int(self._summary.data_peak_from_pixel.text()),
                            int(self._summary.data_peak_to_pixel.text())]

        m.data_x_range = [int(self._summary.x_min_edit.text()),\
                     int(self._summary.x_max_edit.text())]
        m.data_x_range_flag = self._summary.data_low_res_range_switch.isChecked()

        m.norm_x_range = [int(self._summary.norm_x_min_edit.text()),
                          int(self._summary.norm_x_max_edit.text())]
        m.norm_x_range_flag = self._summary.norm_low_res_range_switch.isChecked()

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
        m.TofRangeFlag = self._summary.tof_range_switch.isChecked()

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

        # Scattering angle
        if self._summary.center_pix_radio.isChecked():
            if hasattr(m, "center_pixel"):
                m.center_pixel = float(self._summary.center_pix_edit.text())
                m.use_center_pixel = True
        else:
            if hasattr(m, "theta"):
                m.theta = float(self._summary.angle_edit.text())
                m.use_center_pixel = False

        if hasattr(m, "set_detector_angle"):
            m.set_detector_angle = self._summary.det_angle_check.isChecked()
            m.detector_angle = util._check_and_get_float_line_edit(self._summary.det_angle_edit)
            m.set_detector_angle_offset = self._summary.det_angle_offset_check.isChecked()
            m.detector_angle_offset = util._check_and_get_float_line_edit(self._summary.det_angle_offset_edit)
            m.set_direct_pixel = self._summary.direct_pixel_check.isChecked()
            m.direct_pixel = util._check_and_get_float_line_edit(self._summary.direct_pixel_edit)

        ##
        # Add here states that are data file dependent
        ##

        return m
