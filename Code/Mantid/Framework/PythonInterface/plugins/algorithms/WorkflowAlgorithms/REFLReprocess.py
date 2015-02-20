#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os
import math
import sys

class REFLOptions(object):
    def __init__(self):
        from reduction_gui.reduction.reflectometer.refl_data_script import DataSets as REFLDataSets
        from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
        self._state = DataSeries(data_class=REFLDataSets)
    def get_state(self):
        return self._state

class REFLReprocess(PythonAlgorithm):
    """
        Normalise detector counts by accelerator current and beam spectrum.
    """

    def category(self):
        return "Workflow\\REFL"

    def name(self):
        return "REFLReprocess"

    def summary(self):
        return "Re-reduce REFL data for an entire experiment using saved parameters"

    def PyInit(self):
        self.declareProperty("IPTS", '0', "IPTS number to process")
        self.declareProperty(FileProperty(name="OutputDirectory",defaultValue="",action=FileAction.OptionalDirectory))
        self.declareProperty("LoadProcessed", False, "If True, data will be loaded instead of being processed")
        self.declareProperty("Filter", "ts.txt", "String that the reduced data file name must contain")

    def PyExec(self):
        from reduction_gui.reduction.reflectometer.refl_reduction import REFLReductionScripter
        ipts = self.getProperty("IPTS").value
        try:
            ipts_number = int(ipts)
            ipts = "IPTS-%s" % ipts_number
        except:
            pass

        Logger("REFLReprocess").notice("Processing %s" % ipts)

        # Locate the IPTS directory
        ipts_dir = "/SNS/REF_L/%s/shared" % ipts
        if not os.path.isdir(ipts_dir):
            ipts_dir = ipts

        # Determine the output directory
        output_dir = self.getProperty("OutputDirectory").value
        if len(output_dir)==0:
            output_dir = ipts_dir

        load_only = self.getProperty("LoadProcessed").value
        if load_only:
            return self.load_processed(output_dir)

        # Reprocess the data
        if os.path.isdir(ipts_dir):
            for item in os.listdir(ipts_dir):
                if item.endswith('.xml'):
                    try:
                        Logger("REFLReprocess").notice("Processing %s" % os.path.join(ipts_dir, item))

                        refl = REFLReductionScripter()
                        options = REFLOptions()
                        refl.attach(options)
                        refl.from_xml(os.path.join(ipts_dir, item))
                        code = refl.to_script()
                        exec(code, globals(), locals())
                        self.stitch_data(os.path.join(ipts_dir, item), output_dir,
                                         q_min=options.get_state().data_sets[0].q_min,
                                         q_step=options.get_state().data_sets[0].q_step)
                    except:
                        Logger("REFLReprocess").error(str(sys.exc_value))
        else:
            Logger("REFLReprocess").error("%s not a valid directory" % ipts_dir)

    def load_processed(self, output_dir):
        filter_string = self.getProperty("Filter").value
        if not os.path.isdir(output_dir):
            Logger("REFLReprocess").error("%s not a valid directory" % output_dir)
            return

        for item in os.listdir(output_dir):
            if item.endswith('.txt') and \
               (len(filter_string)==0 or item.find(filter_string)>=0):
                basename, ext = os.path.splitext(item)
                Load(Filename=os.path.join(output_dir, item), OutputWorkspace=basename)
                (_name,_ts) = basename.split('_#')
                CloneWorkspace(InputWorkspace=basename, OutputWorkspace=_name)


    def stitch_data(self, input_file, output_dir, q_min, q_step):
        from LargeScaleStructures.data_stitching import DataSet, Stitcher, RangeSelector
        # Identify the data sets to stitch and order them
        workspace_list = []
        _list_name = []
        _list_ts = []
        ws_list = AnalysisDataService.getObjectNames()
        for item in ws_list:
            if item.endswith('ts'):
                (_name,_ts) = item.split('_#')
                _list_name.append(item)
                _list_ts.append(_ts)

        _name_ts = zip(_list_ts, _list_name)
        _name_ts.sort()
        _ts_sorted, workspace_list = zip(*_name_ts)

        # Stitch the data
        s = Stitcher()

        q_max = 0
        for item in workspace_list:
            data = DataSet(item)
            data.load(True, True)
            x_min, x_max = data.get_range()
            if x_max > q_max:
                q_max = x_max
            s.append(data)

        s.set_reference(0)
        s.compute()

        # Apply the scaling factors
        for data in s._data_sets:
            Scale(InputWorkspace=str(data), OutputWorkspace=data._ws_scaled,
                  Operation="Multiply", Factor=data.get_scale())
            SaveAscii(InputWorkspace=str(data), Filename=os.path.join(output_dir, '%s.txt' % str(data)))


        output_file = input_file.replace('.xml', '_reprocessed.txt')
        Logger("REFLReprocess").notice("Saving to %s" % output_file)


        output_ws = _average_y_of_same_x_(q_min, q_step, q_max)
        SaveAscii(InputWorkspace=output_ws, Filename=output_file)

def weightedMean(data_array, error_array):
    """
        Code taken out as-is from base_ref_reduction.py
    """
    sz = len(data_array)

    # calculate the numerator of mean
    dataNum = 0
    for i in range(sz):
        if not (data_array[i] == 0):
            tmpFactor = float(data_array[i]) / float((pow(error_array[i],2)))
            dataNum += tmpFactor

    # calculate denominator
    dataDen = 0
    for i in range(sz):
        if not (error_array[i] == 0):
            tmpFactor = 1./float((pow(error_array[i],2)))
            dataDen += tmpFactor

    if dataDen == 0:
        mean = 0
        mean_error = 0
    else:
        mean = float(dataNum) / float(dataDen)
        mean_error = math.sqrt(1/dataDen)

    return [mean, mean_error]


def _average_y_of_same_x_(q_min, q_step, q_max=2):
    """

    Code taken out as-is from base_ref_reduction.py

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
    #_from_q = str(state.data_sets[0].q_min)
    #_bin_size = str(state.data_sets[0].q_step)
    #_bin_max = str(2)
    #binning_parameters = _from_q + ',-' + _bin_size + ',' + _bin_max
    binning_parameters = "%s,-%s,%s" % (q_min, q_step, q_max)

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
        data_e_i = mtd[scaled_ws_list[i]+'_histo'].dataE(0)
        for j in range(len(data_y_i)):
            if data_y[j]>0 and data_y_i[j]>0:
                [data_y[j], data_e[j]] = weightedMean([data_y[j], data_y_i[j]], [data_e[j], data_e_i[j]])
            elif (data_y[j] == 0) and (data_y_i[j]>0):
                data_y[j] = data_y_i[j]
                data_e[j] = data_e_i[j]

    return scaled_ws_list[0]+'_histo'

#############################################################################################

AlgorithmFactory.subscribe(REFLReprocess)
