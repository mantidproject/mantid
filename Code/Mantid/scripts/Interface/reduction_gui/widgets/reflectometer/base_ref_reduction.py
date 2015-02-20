#pylint: disable=invalid-name,unused-import
from PyQt4 import QtGui, QtCore
import reduction_gui.widgets.util as util
import math
import os
import time
import sys
from numpy import NAN
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
#from launch_peak_back_selection_1d import DesignerMainWindow

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    import mantid
    #from MantidFramework import *
    #mtd.initialise(False)
    #from mantidsimple import *
    from mantid.simpleapi import *
    from reduction.instruments.reflectometer import data_manipulation

    IS_IN_MANTIDPLOT = True
except:
    pass

class BaseRefWidget(BaseWidget):
    """
        Base widget for reflectivity interfaces
    """
    ## Widget name
    name = "Data"
    instrument_name = ''
    short_name = ''
    peak_pixel_range = []
    background_pixel_range = []

    x_axis = []
    y_axis = []
    e_axis = []

    bDEBUG = False

    def __init__(self, parent=None, state=None, settings=None, name="", data_proxy=None):
        super(BaseRefWidget, self).__init__(parent, state, settings, data_proxy=data_proxy)

        self.short_name = name
        self._settings.instrument_name = name

        # Implement initialization here for each child class

    def initialize_content(self):
        self._summary.edited_warning_label.hide()
        self._summary.log_scale_chk.hide()

        # Validators
        self._summary.data_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_from_pixel))
        self._summary.data_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_to_pixel))
        self._summary.data_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel1))
        self._summary.data_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel1))
        self._summary.data_from_tof.setValidator(QtGui.QIntValidator(self._summary.data_from_tof))
        self._summary.data_to_tof.setValidator(QtGui.QIntValidator(self._summary.data_to_tof))
        self._summary.dq0.setValidator(QtGui.QDoubleValidator(self._summary.dq0))
        self._summary.dq_over_q.setValidator(QtGui.QDoubleValidator(self._summary.dq_over_q))
#        self._summary.overlapValueMeanRadioButton(QtGui.setChecked(False)
#        self._summary.overlapValueLowestErrorRadioButton.setChecked(True)

        self._summary.x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_min_edit))
        self._summary.x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.x_max_edit))
        self._summary.norm_x_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_min_edit))
        self._summary.norm_x_max_edit.setValidator(QtGui.QDoubleValidator(self._summary.norm_x_max_edit))

        self._summary.log_scale_chk.setChecked(True)
        self._summary.q_min_edit.setValidator(QtGui.QDoubleValidator(self._summary.q_min_edit))

        self._summary.angle_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_edit))
        self._summary.angle_offset_error_edit.setValidator(QtGui.QDoubleValidator(self._summary.angle_offset_error_edit))

        self._summary.norm_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_from_pixel))
        self._summary.norm_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_to_pixel))
        self._summary.norm_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_from_pixel1))
        self._summary.norm_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.norm_background_to_pixel1))

        # Event connections
        self.connect(self._summary.data_run_number_edit, QtCore.SIGNAL("returnPressed()"), self.data_run_number_validated)
        self.connect(self._summary.data_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._data_low_res_clicked)
        self.connect(self._summary.norm_low_res_range_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_low_res_clicked)
        self.connect(self._summary.norm_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_clicked)
        self.connect(self._summary.norm_background_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_background_clicked)
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.tof_range_switch, QtCore.SIGNAL("clicked(bool)"), self._tof_range_clicked)
        self.connect(self._summary.plot_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y)
        self.connect(self._summary.plot_count_vs_x_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_x)
        self.connect(self._summary.plot_count_vs_y_bck_btn, QtCore.SIGNAL("clicked()"), self._plot_count_vs_y_bck)

        self.connect(self._summary.plot_data_count_vs_x_2d_btn, QtCore.SIGNAL("clicked(bool)"), self._plot_data_count_vs_x_2d)
        self.connect(self._summary.plot_data_count_vs_tof_2d_btn, QtCore.SIGNAL("clicked(bool)"), self._plot_data_count_vs_tof_2d)

        self.connect(self._summary.norm_count_vs_y_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_y)
        self.connect(self._summary.norm_count_vs_x_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_x)
        self.connect(self._summary.norm_count_vs_y_bck_btn, QtCore.SIGNAL("clicked()"), self._norm_count_vs_y_bck)

        self.connect(self._summary.plot_norm_count_vs_x_2d_btn, QtCore.SIGNAL("clicked(bool)"), self._plot_norm_count_vs_x_2d)
        self.connect(self._summary.plot_norm_count_vs_tof_2d_btn, QtCore.SIGNAL("clicked(bool)"), self._plot_norm_count_vs_tof_2d)

        self.connect(self._summary.plot_tof_btn, QtCore.SIGNAL("clicked()"), self._plot_tof)
        self.connect(self._summary.add_dataset_btn, QtCore.SIGNAL("clicked()"), self._add_data)
        self.connect(self._summary.angle_list, QtCore.SIGNAL("itemSelectionChanged()"), self._angle_changed)
        self.connect(self._summary.remove_btn, QtCore.SIGNAL("clicked()"), self._remove_item)
        self.connect(self._summary.fourth_column_switch, QtCore.SIGNAL("clicked(bool)"), self._fourth_column_clicked)
        self.connect(self._summary.create_ascii_button, QtCore.SIGNAL("clicked()"), self._create_ascii_clicked)
        self.connect(self._summary.use_sf_config_switch, QtCore.SIGNAL("clicked(bool)"), self._use_sf_config_clicked)

        # Catch edited controls
        call_back = partial(self._edit_event, ctrl=self._summary.data_peak_from_pixel)
        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
#        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), self.refresh_data_peak_counts_vs_pixel)

        call_back = partial(self._edit_event, ctrl=self._summary.use_sf_config_switch)
        self.connect(self._summary.use_sf_config_switch, QtCore.SIGNAL("clicked()"), call_back)
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
        self.connect(self._summary.data_run_number_edit, QtCore.SIGNAL("returnPressed()"), self._run_number_changed)
        self._run_number_first_edit = True

        call_back = partial(self._edit_event, ctrl=self._summary.slits_width_flag)
        self.connect(self._summary.slits_width_flag, QtCore.SIGNAL("clicked()"), call_back)

        call_back = partial(self._edit_event, ctrl=self._summary.geometry_correction_switch)
        self.connect(self._summary.geometry_correction_switch, QtCore.SIGNAL("clicked()"), call_back)

        call_back = partial(self._edit_event, ctrl=self._summary.q_min_edit)
        self.connect(self._summary.q_min_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.q_step_edit)
        self.connect(self._summary.q_step_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.log_scale_chk)
        self.connect(self._summary.log_scale_chk, QtCore.SIGNAL("clicked()"), call_back)

        #Incident medium (selection or text changed)
        call_back = partial(self._edit_event, ctrl=self._summary.incident_medium_combobox)
        self.connect(self._summary.incident_medium_combobox, QtCore.SIGNAL("editTextChanged(QString)"), call_back)

        #4th column
        call_back = partial(self._edit_event, ctrl=self._summary.dq0)
        self.connect(self._summary.dq0, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.dq_over_q)
        self.connect(self._summary.dq_over_q, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.fourth_column_switch)
        self.connect(self._summary.fourth_column_switch, QtCore.SIGNAL("clicked()"), call_back)

        #overlap values
        call_back = partial(self._edit_event, ctrl=self._summary.overlapValueMeanRadioButton)
        self.connect(self._summary.overlapValueMeanRadioButton, QtCore.SIGNAL("clicked()"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.overlapValueLowestErrorRadioButton)
        self.connect(self._summary.overlapValueLowestErrorRadioButton, QtCore.SIGNAL("clicked()"), call_back)

        #name of output file changed
        call_back = partial(self._edit_event, ctrl=self._summary.cfg_scaling_factor_file_name)
        self.connect(self._summary.cfg_scaling_factor_file_name_browse, QtCore.SIGNAL("clicked()"), call_back)

        #scaling factor configuration file
        self.connect(self._summary.cfg_scaling_factor_file_name_browse, QtCore.SIGNAL("clicked()"), self.browse_config_file_name)

        # Output directory
        self._summary.outdir_edit.setText(os.path.expanduser('~'))
        self.connect(self._summary.outdir_browse_button, QtCore.SIGNAL("clicked()"), self._output_dir_browse)

        # Set up the automated reduction options
        self._summary.auto_reduce_check.setChecked(False)
        self._auto_reduce(False)
        self.connect(self._summary.auto_reduce_check, QtCore.SIGNAL("clicked(bool)"), self._auto_reduce)
        self.connect(self._summary.auto_reduce_btn, QtCore.SIGNAL("clicked()"), self._create_auto_reduce_template)

        # If we do not have access to /SNS, don't display the automated reduction options
        if not self._settings.debug and not os.path.isdir("/SNS/%s" % self.instrument_name):
            self._summary.auto_reduce_check.hide()

    def getMetadata(self,file):
        """
        This retrieve the metadata from the data event NeXus file
        """
        _full_file_name = file
        tmpWks = LoadEventNexus(Filename=_full_file_name,MetaDataOnly='1')

        #mt1 = mtd['tmpWks']
        #mt_run = mt1.getRun()
        mt_run = tmpWks.getRun()

        #tthd
        tthd = mt_run.getProperty('tthd').value[0]

        #ths
        ths = mt_run.getProperty('ths').value[0]

        #lambda requested
        lambda_requested = mt_run.getProperty('LambdaRequest').value[0]

        #s1h, s2h, s1w and s2w
        s1h = mt_run.getProperty('S1VHeight').value[0]
        s2h = mt_run.getProperty('S2VHeight').value[0]
        try:
            s1w = mt_run.getProperty('S1HWidth').value[0]
        except:
            s1w = 'N/A'
        s2w = mt_run.getProperty('S2HWidth').value[0]

        return [tthd,ths, lambda_requested, s1h, s2h, s1w, s2w]

    def data_run_number_validated(self):
        """
        This function is reached when the user hits ENTER on the data run
        number and will retrieve some of the metadata and display them
        in the metadata box
        """
#        self._summary.data_run_number_processing.show()
        run_number = self._summary.data_run_number_edit.text()

        try:
            _file = FileFinder.findRuns("REF_L%d"%int(run_number))[0]
            lambdaRequest = ''

            metadata= self.getMetadata(_file)

            #tthd
            tthd_value = metadata[0]
            tthd_value_string = '{0:.2f}'.format(tthd_value)
            self._summary.tthd_value.setText(tthd_value_string)

            #ths
            ths_value = metadata[1]
            ths_value_string = '{0:.2f}'.format(ths_value)
            self._summary.ths_value.setText(ths_value_string)

            #lambda requested
            lambda_value = metadata[2]
            lambda_value_string = '{0:.2f}'.format(lambda_value)
            self._summary.lambda_request.setText(lambda_value_string)

            #s1h, s2h, s1w and s2w
            s1h_value = metadata[3]
            s1h_value_string = '{0:.2f}'.format(s1h_value)
            self._summary.s1h.setText(s1h_value_string)

            s2h_value = metadata[4]
            s2h_value_string = '{0:.2f}'.format(s2h_value)
            self._summary.s2h.setText(s2h_value_string)

            s1w_value = metadata[5]
            try:
                s1w_value_string = '{0:.2f}'.format(s1w_value)
            except:
                s1w_value_string = 'N/A'
            self._summary.s1w.setText(s1w_value_string)

            s2w_value = metadata[6]
            s2w_value_string = '{0:.2f}'.format(s2w_value)
            self._summary.s2w.setText(s2w_value_string)

#            self._summary.data_run_number_processing.hide()
        except:
            pass
#            self._summary.data_run_number_processing.hide()

    def _output_dir_browse(self):
        output_dir = QtGui.QFileDialog.getExistingDirectory(self, "Output Directory - Choose a directory",
                                                            os.path.expanduser('~'),
                                                            QtGui.QFileDialog.ShowDirsOnly
                                                            | QtGui.QFileDialog.DontResolveSymlinks)
        if output_dir:
            self._summary.outdir_edit.setText(output_dir)

    def _smooth_x_axis(self, x_axis, y_axis, e_axis):
        """
        This method will do an weighted average of the value when
        their x-axis is within a given range (precision)
        """
        _precision = 0.1/100.   #0.1%

        new_x_axis = []
        new_y_axis = []
        new_e_axis = []

#        if self.bDEBUG:
#            print 'x_axis before _smooth_x_axis:'
#            print x_axis

        sz = len(x_axis)
        i=0
        while i < sz-1:

            _left_x = x_axis[i]
            _right_x = x_axis[i+1]

            bCalAverage = False
            if _left_x == _right_x:
                bCalAverage = True
            else:
                _left_x = math.fabs(_left_x)
                _right_x = math.fabs(_right_x)
                _relative_diff = (_left_x - _right_x) / (_left_x + _right_x)
                if math.fabs(_relative_diff <= _precision):
                    bCalAverage = True

            _left_e = e_axis[i]
            _left_e2 = _left_e * _left_e
            _left_y = y_axis[i]

            if bCalAverage:

                #average the two values
                _right_e = e_axis[i+1]
                _right_e2 = _right_e * _right_e
                _right_y = y_axis[i+1]

                if _left_e2 == 0. or _right_e2 == 0.:
                    _y = 0.
                    _e = 0.
                else:

                    _error = 1./_left_e2 + 1./_right_e2
                    _x = (_left_x + _right_x) / 2.
                    _y = (_left_y/_left_e2 + _right_y/_right_e2) / _error
                    _e = math.sqrt(1./_error)

                new_x_axis.append(_x)
                new_y_axis.append(_y)
                new_e_axis.append(_e)

                i+=1

            else:

                new_x_axis.append(x_axis[i])
                new_y_axis.append(y_axis[i])
                new_e_axis.append(e_axis[i])

            i+=1

#        if self.bDEBUG:
#            print
#            print 'x-axis after _smooth_x_axis:'
#            print new_x_axis

        self.x_axis = new_x_axis
        self.y_axis = new_y_axis
        self.e_axis = new_e_axis

    def weightedMean(self, data_array, error_array):

        sz = len(data_array)

        # calculate the numerator of mean
        dataNum = 0
        for i in range(sz):
            if not data_array[i] == 0:
                tmpFactor = float(data_array[i]) / float((pow(error_array[i],2)))
                dataNum += tmpFactor

        # calculate denominator
        dataDen = 0
        for i in range(sz):
            if not error_array[i] == 0:
                tmpFactor = 1./float((pow(error_array[i],2)))
                dataDen += tmpFactor

        if dataDen == 0:
            mean = 0
            mean_error = 0
        else:
            mean = float(dataNum) / float(dataDen)
            mean_error = math.sqrt(1/dataDen)

        return [mean, mean_error]

    def _average_y_of_same_x_(self):
        """
        2 y values sharing the same x-axis will be average using
        the weighted mean
        """

        ws_list = AnalysisDataService.getObjectNames()
        scaled_ws_list = []

        # Get the list of scaled histos
        for ws in ws_list:
            if ws.endswith("_scaled"):
                scaled_ws_list.append(ws)


        # get binning parameters
        _from_q = str(self._summary.q_min_edit.text())
        _bin_size = str(self._summary.q_step_edit.text())
        _bin_max = str(2)
        binning_parameters = _from_q + ',-' + _bin_size + ',' + _bin_max

        # Convert each histo to histograms and rebin to final binning
        for ws in scaled_ws_list:
            new_name = "%s_histo" % ws
            ConvertToHistogram(InputWorkspace=ws, OutputWorkspace=new_name)
            Rebin(InputWorkspace=new_name, Params=binning_parameters,
                  OutputWorkspace=new_name)

        # Take the first rebinned histo as our output
        data_y = mtd[scaled_ws_list[0]+'_histo'].dataY(0)
        data_e = mtd[scaled_ws_list[0]+'_histo'].dataE(0)

        # Add in the other histos, averaging the overlaps
        for i in range(1, len(scaled_ws_list)):
            data_y_i = mtd[scaled_ws_list[i]+'_histo'].dataY(0)
            data_e_i = mt[scaled_ws_list[i]+'_histo'].dataE(0)
            for j in range(len(data_y_i)):

                if data_y[j]>0 and data_y_i[j]>0:
                    [data_y[j], data_e[j]] = self.weightedMean([data_y[j], data_y_i[j]], [data_e[j], data_e_i[j]])
                elif (data_y[j] == 0) and (data_y_i[j]>0):
                    data_y[j] = data_y_i[j]
                    data_e[j] = data_e_i[j]

        return scaled_ws_list[0]+'_histo'

    def _produce_y_of_same_x_(self, isUsingLessErrorValue):
        """
        2 y values sharing the same x-axis will be average using
        the weighted mean
        """

        ws_list = AnalysisDataService.getObjectNames()
        scaled_ws_list = []

        # Get the list of scaled histos
        for ws in ws_list:
            if ws.endswith("_scaled"):
                scaled_ws_list.append(ws)

        # get binning parameters
        _from_q = str(self._summary.q_min_edit.text())
        _bin_size = str(self._summary.q_step_edit.text())
        _bin_max = str(2)
        binning_parameters = _from_q + ',-' + _bin_size + ',' + _bin_max

        ## DEBUGGING ONLY
        file_number = 0
#        print '=========== BEFORE REBINING =========='
        for ws in scaled_ws_list:
#            print 'file_number: ' , file_number
            data_y = mtd[ws].dataY(0)
            data_e = mtd[ws].dataE(0)

            # cleanup data 0-> NAN
            for j in range(len(data_y)):
#                print '-> data_y[j]: ' , data_y[j] , ' data_e[j]: ' , data_y[j]
                if data_y[j] < 1e-12:
                    data_y[j] = NAN
                    data_e[j] = NAN

            file_number = file_number + 1

        ## END OF DEBUGGING ONLY

        # Convert each histo to histograms and rebin to final binning
        for ws in scaled_ws_list:
            new_name = "%s_histo" % ws
            ConvertToHistogram(InputWorkspace=ws, OutputWorkspace=new_name)
#            mtd[new_name].setDistribution(True)
            Rebin(InputWorkspace=new_name, Params=binning_parameters,
                  OutputWorkspace=new_name)

        # Take the first rebinned histo as our output
        data_y = mtd[scaled_ws_list[0]+'_histo'].dataY(0)
        data_e = mtd[scaled_ws_list[0]+'_histo'].dataE(0)

        # skip first 3 points and last one
        skip_index = 0
        point_to_skip = 3

#        print '============ AFTER REBINING ================' #DEBUGGING ONLY

        # Add in the other histos, averaging the overlaps
        for i in range(1, len(scaled_ws_list)):

#            print 'i: ' , i

            skip_point = True
            can_skip_last_point = False

            data_y_i = mtd[scaled_ws_list[i]+'_histo'].dataY(0)
            data_e_i = mtd[scaled_ws_list[i]+'_histo'].dataE(0)
            for j in range(len(data_y_i)-1):

#                print '-> j: ' , j

                if data_y_i[j] > 0:

#                    print '   data_y_i[j]: ', data_y_i[j], ' data_e_i[j]: ' , data_e_i[j]

                    can_skip_last_point = True
                    if skip_point:
                        skip_index = skip_index + 1
                        if skip_index == point_to_skip:
                            skip_point = False
                            skip_index = 0
                        else:
                            continue

                if can_skip_last_point and (data_y_i[j+1]==0):
                    break

                if data_y[j]>0 and data_y_i[j]>0:

                    if isUsingLessErrorValue:
                        if data_e[j] > data_e_i[j]:
                            data_y[j] = data_y_i[j]
                            data_e[j] = data_e_i[j]
                    else:
                        [data_y[j], data_e[j]] = self.weightedMean([data_y[j], data_y_i[j]], [data_e[j], data_e_i[j]])

                elif (data_y[j] == 0) and (data_y_i[j]>0):
                    data_y[j] = data_y_i[j]
                    data_e[j] = data_e_i[j]


        return scaled_ws_list[0]+'_histo'

    def _create_ascii_clicked(self):
        """
        Reached by the 'Create ASCII' button
        """

        #make sure there is the right output workspace called '
#        if not mtd.workspaceExists('ref_combined'):
#            print 'Workspace "ref_combined" does not exist !'
#            return

        #get default output file name
        run_number = self._summary.data_run_number_edit.text()
        default_file_name = 'REFL_' + run_number + '_combined_data.txt'

        #retrieve name of the output file
        file_name = QtGui.QFileDialog.getSaveFileName(self, "Select or define a ASCII file name", default_file_name, "(*.txt)")
        if str(file_name).strip() == '':
            return

        #check the status of the 4th column switch
        _with_4th_flag = self._summary.fourth_column_switch.isChecked()
        text = []
        if _with_4th_flag:
            dq0 = float(self._summary.dq0.text())
            dq_over_q = float(self._summary.dq_over_q.text())
            line1 = '#dQ0[1/Angstrom]=' + str(dq0)
            line2 = '#dQ/Q=' + str(dq_over_q)
            line3 = '#Q(1/Angstrom) R delta_R Precision'
            text = [line1, line2, line3]
        else:
            text = ['#Q(1/Angstrom) R delta_R']

#        #rebinned using output factors
#        q_min = float(self._summary.q_min_edit.text())
#        q_bin = -float(self._summary.q_step_edit.text())
#
#        mt = mtd['ref_combined']
#        x_axis = mt.readX(0)[:]
#        q_max = float(x_axis[-1])
#
#        q_binning = [q_min, q_bin, q_max]
#        Rebin(InputWorkspace='ref_combined',
#              OutputWorkspace='ref_combined',
#              Params=q_binning)

        #using mean or value with less error
        _overlap_less_error_flag = self._summary.overlapValueLowestErrorRadioButton.isChecked()
        wks_file_name = self._produce_y_of_same_x_(_overlap_less_error_flag)
#        print 'wks_file_name: ' , wks_file_name

#        mt = mtd['ref_combined']
#        x_axis = mt.readX(0)[:]
#        y_axis = mt.readY(0)[:]
#        e_axis = mt.readE(0)[:]
#
#        self._smooth_x_axis(x_axis, y_axis, e_axis)

        x_axis = mtd[wks_file_name].readX(0)[:]
        y_axis = mtd[wks_file_name].readY(0)[:]
        e_axis = mtd[wks_file_name].readE(0)[:]

        sz = len(x_axis)-1
        for i in range(sz):
            # do not display data where R=0
            if y_axis[i] > 1e-15:
                _line = str(x_axis[i])
                _line += ' ' + str(y_axis[i])
                _line += ' ' + str(e_axis[i])
                if _with_4th_flag:
                    _precision = str(dq0 + dq_over_q * x_axis[i])
                    _line += ' ' + _precision
                text.append(_line)

        f=open(file_name,'w')
        for _line in text:
            f.write(_line + '\n')


    def browse_config_file_name(self):
        '''
        Define configuration file name
        '''
        try:
            file_name = QtGui.QFileDialog.getOpenFileName(self, "Select a SF configuration file", "", "(*.cfg)")
            if str(file_name).strip() != '':
                if os.path.isfile(file_name):
                    self._summary.cfg_scaling_factor_file_name.setText(file_name)
                    self.retrieve_list_of_incident_medium(file_name)
        except:
            print 'Invalid file format (' + file_name + ')'

    def variable_value_splitter(self, variable_value):
        """
            This function split the variable that looks like "LambdaRequested:3.75"
            and returns a dictionnary of the variable name and value
        """
        _split = variable_value.split('=')
        variable = _split[0]
        value = _split[1]
        return {'variable':variable, 'value':value}

    def retrieve_list_of_incident_medium(self, cfg_file_name):
        """
        This procedure will parse the configuration file and will
        populate the Incident Medium dropbox with the list of incident medium
        found
        """
        f=open(cfg_file_name,'r')
        text = f.readlines()
        list_incident_medium = []
        for _line in text:
            if _line[0] == '#':
                continue

            _line_split = _line.split(' ')
            _incident_medium = self.variable_value_splitter(_line_split[0])
            list_incident_medium.append(_incident_medium['value'])

        _unique_list = list(set(list_incident_medium))

        self._summary.incident_medium_combobox.clear()
        for i in range(len(_unique_list)):
            self._summary.incident_medium_combobox.addItem(str(_unique_list[i]))

    def _run_number_changed(self):
        self._edit_event(ctrl=self._summary.data_run_number_edit)

    def _edit_event(self, text=None, ctrl=None):
        self._summary.edited_warning_label.show()
        util.set_edited(ctrl,True)

    def _reset_warnings(self):
        self._summary.edited_warning_label.hide()
        util.set_edited(self._summary.use_sf_config_switch, False)
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
        util.set_edited(self._summary.angle_offset_error_edit, False)
        util.set_edited(self._summary.norm_peak_from_pixel, False)
        util.set_edited(self._summary.norm_peak_to_pixel, False)
        util.set_edited(self._summary.norm_background_from_pixel1, False)
        util.set_edited(self._summary.norm_background_to_pixel1, False)
        util.set_edited(self._summary.norm_run_number_edit, False)
        util.set_edited(self._summary.data_run_number_edit, False)
        util.set_edited(self._summary.log_scale_chk, False)
        util.set_edited(self._summary.data_background_switch, False)
        util.set_edited(self._summary.norm_background_switch, False)
        util.set_edited(self._summary.data_low_res_range_switch, False)
        util.set_edited(self._summary.norm_low_res_range_switch, False)
        util.set_edited(self._summary.norm_switch, False)
        #util.set_edited(self._summary.tof_range_switch, False)
        util.set_edited(self._summary.q_min_edit, False)
        util.set_edited(self._summary.q_step_edit, False)
        util.set_edited(self._summary.cfg_scaling_factor_file_name, False)
        util.set_edited(self._summary.incident_medium_combobox, False)
        util.set_edited(self._summary.overlapValueLowestErrorRadioButton, False)
        util.set_edited(self._summary.overlapValueMeanRadioButton, False)
        util.set_edited(self._summary.dq0, False)
        util.set_edited(self._summary.dq_over_q, False)
        util.set_edited(self._summary.fourth_column_switch, False)
        util.set_edited(self._summary.slits_width_flag, False)
        util.set_edited(self._summary.geometry_correction_switch, False)
        util.set_edited(self._summary.angle_offset_edit, False)
        util.set_edited(self._summary.angle_offset_error_edit, False)

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
        super(BaseRefWidget, self).is_running(is_running)
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

    def _use_sf_config_clicked(self, is_checked):
        """
            This is reached by the 'Use SF configuration file'
        """
        self._summary.outdir_label_3.setEnabled(is_checked)
        self._summary.cfg_scaling_factor_file_name.setEnabled(is_checked)
        self._summary.cfg_scaling_factor_file_name_browse.setEnabled(is_checked)
        self._summary.slits_width_flag.setEnabled(is_checked)

    def _plot_count_vs_y(self, is_peak=True):
        """
            Plot counts as a function of high-resolution pixels
            and select peak range
            For REFM, this is X
            For REFL, this is Y
        """

#        run_number = self._summary.data_run_number_edit.text()
#        f = FileFinder.findRuns("%s%s" % (self.instrument_name, str(run_number)))[0]
#
#        #range_min = int(min_ctrl.text())
#        #range_max = int(max_ctrl.text())
#
#        # For REFL, Y is high-res
#        is_pixel_y = True
#        is_high_res = True
#        isPeak = True
#        min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
#                                                                      high_res=is_high_res,
#                                                                      instrument=self.short_name,
#                                                                      isPeak=isPeak)
#
#        # for low res
#        is_high_res = False
#        is_pixel_y = False
#        min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
#                                                                      high_res=is_high_res,
#                                                                      instrument=self.short_name,
#                                                                      isPeak=isPeak)
#
#
#        basename = os.path.basename(f)
#        wk1 = "Peak - " + basename + " - Y pixel "
#        wk2 = "Peak - " + basename + " - X pixel "
#        dmw = DesignerMainWindow(parent=self, wk1=wk1, wk2=wk2, type='data')
#
#        # show it
#        dmw.show()
#        # start the Qt main loop execution, existing from this script
#        # with the same return code of Qt application
##        sys.exit(app.exec_())

        min, max = self._integrated_plot(True,
                                         self._summary.data_run_number_edit,
                                         self._summary.data_peak_from_pixel,
                                         self._summary.data_peak_to_pixel,
                                         True)

    def _plot_count_vs_y_bck(self):
        """
            Plot counts as a function of high-resolution pixels
            and select background range
            For REFM, this is X
            For REFL, this is Y
        """

        min, max = self._integrated_plot(True,\
                              self._summary.data_run_number_edit,\
                              self._summary.data_background_from_pixel1,\
                              self._summary.data_background_to_pixel1,\
                              False)

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

    def _plot_data_count_vs_x_2d(self):
        """
            Will launch the 2d plot for the data of counts vs x
        """
        return


    def _plot_data_count_vs_tof_2d(self):
        """
            Will launch the 2d plot for the data of counts vs TOF
        """

        #retrieve name of workspace first
        run_number =  self._summary.data_run_number_edit.text()
        file_path = FileFinder.findRuns("%s%s" % (self.instrument_name, str(run_number)))[0]

        basename = os.path.basename(file_path)
        ws_base = "__%s" % basename

        if self.instrument_name == 'REF_L':
            ws_output_base = "Pixel Y vs TOF" + " - " + basename
        else:
            ws_output_base = "Pixel X vs TOF" + " - " + basename

#        if (self.instrument_name == 'REF_L'):
#            if isPeak:
#                type = 'Peak'
#            else:
#                type = 'Background'
#            if is_pixel_y is False:
#                x_title = "X pixel"
#            else:
#                x_title = "Y pixel"
#            ws_output_base =  type + " - " + basename + " - " + x_title
#        else:
#            ws_output_base = "Counts vs TOF - %s" % basename
#            x_title = "Y pixel"
#            if is_pixel_y is False:
#                ws_output_base = "Counts vs X pixel - %s" % basename
#                x_title = "X pixel"

        range_min = int(self._summary.data_from_tof.text())
        range_max = int(self._summary.data_to_tof.text())

        ws_output_base = "Peak - " + basename + " - Y pixel _2D"
#        if mtd.workspaceExists(ws_output_base):
#            mtd.deleteWorkspace(ws_output_base)
#            ws_output_base_1 = "__" + self.instrument_name + "_" + str(run_number) + "_event.nxs"
#            mtd.deleteWorkspace(ws_output_base_1)
#            ws_output_base_2 = "__" + self.instrument_name + "_" + str(run_number) + "_event.nxs_all"
#            mtd.deleteWorkspace(ws_output_base_2)
#            ws_output_base_3 = "Peak - " + self.instrument_name + "_" + str(run_number) + "_event.nxs - Y pixel "
#            mtd.deleteWorkspace(ws_output_base_3)

        if mtd.doesExist(ws_output_base):
            DeleteWorkspace(ws_output_base)
            ws_output_base_1 = "__" + self.instrument_name + "_" + str(run_number) + "_event.nxs"
            DeleteWorkspace(ws_output_base_1)
            ws_output_base_2 = "__" + self.instrument_name + "_" + str(run_number) + "_event.nxs_all"
            DeleteWorkspace(ws_output_base_2)
            ws_output_base_3 = "Peak - " + self.instrument_name + "_" + str(run_number) + "_event.nxs - Y pixel "
            DeleteWorkspace(ws_output_base_3)

        data_manipulation.counts_vs_pixel_distribution(file_path,
                                                       is_pixel_y=True,
                                                       callback=None,
                                                       instrument='REFL')

#        def call_back(peakmin, peakmax, backmin, backmax, tofmin, tofmax):
#            print 'Inside the call_back on the python side'
#            self._summary.data_peak_from_pixel.setText("%-d" % int(peakmin))
#            self._summary.data_peak_to_pixel.setText("%-d" % int(peakmax))
#            self._summary.data_background_from_pixel1.setText("%-d" % int(backmin))
#            self._summary.data_background_to_pixel1.setText("%-d" % int(backmax))
#            self._summary.x_min_edit.setText("%-d" % int(tofmin))
#            self._summary.x_max_edit.setText("%-d" % int(tofmax))

        # mantidplot.app should be used instead of _qti.app (it's just an alias)
        #mantidplot.app.connect(mantidplot.app.mantidUI, QtCore.SIGNAL("python_peak_back_tof_range_update(double,double,double,double,double,double)"), call_back)
        #mantidplot.app.connect(mantidplot.app.RefDetectorViewer, QtCore.SIGNAL("python_peak_back_tof_range_update(double,double,double,double,double,double)"), call_back)

        peak_min = int(self._summary.data_peak_from_pixel.text())
        peak_max = int(self._summary.data_peak_to_pixel.text())
        back_min = int(self._summary.data_background_from_pixel1.text())
        back_max = int(self._summary.data_background_to_pixel1.text())
        tof_min = int(self._summary.data_from_tof.text())
        tof_max = int(self._summary.data_to_tof.text())

        import mantidqtpython
        self.ref_det_view = mantidqtpython.MantidQt.RefDetectorViewer.RefMatrixWSImageView(ws_output_base, peak_min, peak_max, back_min, back_max, tof_min, tof_max)
        QtCore.QObject.connect(self.ref_det_view.getConnections(),
                               QtCore.SIGNAL("peak_back_tof_range_update(double,double, double,double,double,double)"), self.call_back)


    def call_back(self, peakmin, peakmax, backmin, backmax, tofmin, tofmax):
        self._summary.data_peak_from_pixel.setText("%-d" % int(peakmin))
        self._summary.data_peak_to_pixel.setText("%-d" % int(peakmax))
        self._summary.data_background_from_pixel1.setText("%-d" % int(backmin))
        self._summary.data_background_to_pixel1.setText("%-d" % int(backmax))
        self._summary.data_from_tof.setText("%-d" % int(tofmin))
        self._summary.data_to_tof.setText("%-d" % int(tofmax))

    def _norm_count_vs_y(self):

#        run_number = self._summary.norm_run_number_edit.text()
#        f = FileFinder.findRuns("%s%s" % (self.instrument_name, str(run_number)))[0]
#
#        #range_min = int(min_ctrl.text())
#        #range_max = int(max_ctrl.text())
#
#        # For REFL, Y is high-res
#        is_pixel_y = True
#        is_high_res = True
#        isPeak = True
#        min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
#                                                                      high_res=is_high_res,
#                                                                      instrument=self.short_name,
#                                                                      isPeak=isPeak)
#
#        # for low res
#        is_high_res = False
#        is_pixel_y = False
#        min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
#                                                                      high_res=is_high_res,
#                                                                      instrument=self.short_name,
#                                                                      isPeak=isPeak)
#
#
#        basename = os.path.basename(f)
#        wk1 = "Peak - " + basename + " - Y pixel "
#        wk2 = "Peak - " + basename + " - X pixel "
#        dmw = DesignerMainWindow(parent=self, wk1=wk1, wk2=wk2, type='norm')
#
#        # show it
#        dmw.show()

#        dmw = DesignerMainWindow(self, 'norm')
#        dmw.show()
        min, max = self._integrated_plot(True,
                                         self._summary.norm_run_number_edit,
                                         self._summary.norm_peak_from_pixel,
                                         self._summary.norm_peak_to_pixel)

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

    def _plot_norm_count_vs_x_2d(self):
        """
            Will launch the 2d plot for the norm of counts vs x
        """
        return


    def _plot_norm_count_vs_tof_2d(self):
        """
            Will launch the 2d plot for the norm of counts vs TOF
        """
        return



    def _integrated_plot(self, is_high_res, file_ctrl, min_ctrl, max_ctrl, isPeak=True):
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
            @param isPeak: are we working with peak or with background
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

            is_pixel_y =  is_high_res
            # For REFM it's the other way around
            if self.short_name == "REFM":
                is_pixel_y = not is_pixel_y

            min, max = data_manipulation.counts_vs_pixel_distribution(f, is_pixel_y=is_pixel_y,
                                                                      callback=call_back,
                                                                      range_min=range_min,
                                                                      range_max=range_max,
                                                                      high_res=is_high_res,
                                                                      instrument=self.short_name,
                                                                      isPeak=isPeak)

            return min, max
        except:
            pass

    def _plot_tof(self):
        if not IS_IN_MANTIDPLOT:
            return
        try:
            f = FileFinder.findRuns("%s%s" % (self.instrument_name, str(self._summary.norm_run_number_edit.text())))[0]
#            print FileFinder.findRuns("%s%s" % (self.instrument_name, str(self._summary.norm_run_number_edit.text())))
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

            #loop over all the already defined states and give all of them the
            #same Qmin, Qsteps, Angle offset, scaling factor config file name
            #and incident medium
            i=0
            while i < self._summary.angle_list.count():

                current_item = self._summary.angle_list.item(i)
                state = current_item.data(QtCore.Qt.UserRole)

                _q_min = self._summary.q_min_edit.text()
                state.q_min = float(_q_min)
                _q_step = self._summary.q_step_edit.text()
                if self._summary.log_scale_chk.isChecked():
                    _q_step = '-' + _q_step
                state.q_step = float(_q_step)

                state.scaling_factor_file = self._summary.cfg_scaling_factor_file_name.text()
                if self._summary.use_sf_config_switch.isChecked():
                    state.scaling_factor_file_flag = True
                else:
                    state.scaling_factor_file_flag = False

                state.slits_width_flag = self._summary.slits_width_flag.isChecked()

                state.geometry_correction_switch = self._summary.geometry_correction_switch.isChecked()

                #incident medium
                _incident_medium_list = [str(self._summary.incident_medium_combobox.itemText(j))\
                                          for j in range(self._summary.incident_medium_combobox.count())]
                _incident_medium_index_selected = self._summary.incident_medium_combobox.currentIndex()

                _incident_medium_string = (',').join(_incident_medium_list)
                state.incident_medium_list = [_incident_medium_string]

                state.incident_medium_index_selected = _incident_medium_index_selected

                # how to treat overlap values
                state.overlap_lowest_error = self._summary.overlapValueLowestErrorRadioButton.isChecked()
                state.overlap_mean_value = self._summary.overlapValueMeanRadioButton.isChecked()

                #4th column (precision)
                state.fourth_column_dq0 = self._summary.dq0.text()
                state.fourth_column_dq_over_q = self._summary.dq_over_q.text()

                current_item.setData(QtCore.Qt.UserRole, state)
                i+=1

        else:
            item_widget = QtGui.QListWidgetItem(run_numbers, self._summary.angle_list)
            state.scaling_factor_file = self._summary.cfg_scaling_factor_file_name.text()

            if self._summary.use_sf_config_switch.isChecked():
                state.scaling_factor_file_flag = True
            else:
                state.scaling_factor_file_flag = False

                state.geometry_correction_switch = self._summary.geometry_correction_switch.isChecked()

             #incident medium
            _incident_medium_list = [str(self._summary.incident_medium_combobox.itemText(j))
                                     for j in range(self._summary.incident_medium_combobox.count())]
            _incident_medium_index_selected = self._summary.incident_medium_combobox.currentIndex()

            _incident_medium_string = (',').join(_incident_medium_list)
            state.incident_medium_list = [_incident_medium_string]

            # how to treat overlap values
            state.overlap_lowest_error = self._summary.overlapValueLowestErrorRadioButton.isChecked()
            state.overlap_mean_value = self._summary.overlapValueMeanRadioButton.isChecked()

            state.incident_medium_index_selected = _incident_medium_index_selected

            item_widget.setData(QtCore.Qt.UserRole, state)

        # Read logs
        if not in_list and self.short_name == "REFM":
            self._read_logs()

        self._reset_warnings()

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

#            # Common Q binning
#            self._summary.q_min_edit.setText(str(state.data_sets[0].q_min))
#            self._summary.log_scale_chk.setChecked(state.data_sets[0].q_step<0)

            # Common angle offset
            if hasattr(state.data_sets[0], "angle_offset"):
                self._summary.angle_offset_edit.setText(str(state.data_sets[0].angle_offset))
                self._summary.angle_offset_error_edit.setText(str(state.data_sets[0].angle_offset_error))

        self._reset_warnings()

    def set_editing_state(self, state):
        self._summary.q_min_edit.setText(str(state.q_min))
        self._summary.log_scale_chk.setChecked(state.q_step<0)

        self._summary.incident_medium_combobox.clear()
        _incident_medium_str = str(state.incident_medium_list[0])
        _list = _incident_medium_str.split(',')
        for i in range(len(_list)):
            self._summary.incident_medium_combobox.addItem(str(_list[i]))
        self._summary.incident_medium_combobox.setCurrentIndex(state.incident_medium_index_selected)

        #Peak from/to pixels
        self._summary.data_peak_from_pixel.setText(str(state.DataPeakPixels[0]))
        self._summary.data_peak_to_pixel.setText(str(state.DataPeakPixels[1]))

        #data low resolution range
        self._summary.data_low_res_range_switch.setChecked(state.data_x_range_flag)
        self._summary.x_min_edit.setText(str(state.data_x_range[0]))
        self._summary.x_max_edit.setText(str(state.data_x_range[1]))
        self._data_low_res_clicked(state.data_x_range_flag)

        #data metadata
        self._summary.tthd_value.setText(str(state.tthd_value))
        self._summary.ths_value.setText(str(state.ths_value))

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
        self._summary.q_step_edit.setText(str(math.fabs(state.q_step)))

        # overlap ascii values
        self._summary.overlapValueLowestErrorRadioButton.setChecked(state.overlap_lowest_error)
        self._summary.overlapValueMeanRadioButton.setChecked(state.overlap_mean_value)

        # Output directory
        if hasattr(state, "output_dir"):
            if len(str(state.output_dir).strip())>0:
                self._summary.outdir_edit.setText(str(state.output_dir))

        #scaling factor file and options
        self._summary.use_sf_config_switch.setChecked(state.scaling_factor_file_flag)
        self._summary.cfg_scaling_factor_file_name.setText(str(state.scaling_factor_file))
        self._summary.slits_width_flag.setChecked(state.slits_width_flag)
        self._use_sf_config_clicked(state.scaling_factor_file_flag)

        # geomery correction
        self._summary.geometry_correction_switch.setChecked(state.geometry_correction_switch)

        self._reset_warnings()
        self._summary.data_run_number_edit.setText(str(','.join([str(i) for i in state.data_files])))

        #4th column (precision)
        self._summary.fourth_column_switch.setChecked(state.fourth_column_flag)
        self._fourth_column_clicked(state.fourth_column_flag)
