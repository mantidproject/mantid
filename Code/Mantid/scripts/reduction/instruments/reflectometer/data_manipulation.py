#pylint: disable=invalid-name
import os
from mantid.simpleapi import *

def tof_distribution(file_path, callback=None,
                     range_min=None, range_max=None):
    """
        Plot counts as a function of TOF for a given REF_L data file
    """

    print 'entering tof_distribution'

    basename = os.path.basename(file_path)
    ws_raw = "__%s" % basename
    ws = "__TOF_distribution"

#     if not mtd.workspaceExists(ws_raw):
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws_raw)

    Rebin(InputWorkspace=ws_raw, OutputWorkspace=ws,Params="0,200,200000")
    SumSpectra(InputWorkspace=ws, OutputWorkspace=ws)

    # Get range of TOF where we have data
    x = mtd[ws].readX(0)
    y = mtd[ws].readY(0)
    xmin = x[0]
    xmax = None
    for i in range(len(y)):
        if y[i]==0.0 and xmax is None:
            xmin = x[i]
        if y[i]>0:
            xmax = x[i]

    if callback is not None:
        from LargeScaleStructures import data_stitching
        data_stitching.RangeSelector.connect([ws], callback,
                                             xmin=xmin, xmax=xmax,
                                             range_min=range_min,
                                             range_max=range_max)


def counts_vs_pixel_distribution(file_path, is_pixel_y=True, callback=None,
                                 range_min=None, range_max=None,
                                 high_res = True, instrument="REFL",
                                 isPeak=True,
                                 tof_min=None, tof_max=None):
    """
        Display counts vs pixel of data or normalization data

        @param isPeak: are we working with peak or with background

    """
    basename = os.path.basename(file_path)
    ws_base = "__%s" % basename

    ws_output_base = ''
    if instrument == 'REFL':
        if isPeak:
            type = 'Peak'
        else:
            type = 'Background'
        if is_pixel_y is False:
            x_title = "X pixel"
        else:
            x_title = "Y pixel"
        ws_output_base =  type + " - " + basename + " - " + x_title
    else:
        ws_output_base = "Counts vs Y pixel - %s" % basename
        x_title = "Y pixel"
        if is_pixel_y is False:
            ws_output_base = "Counts vs X pixel - %s" % basename
            x_title = "X pixel"

    ws_list = []

    if tof_min is None:
        tof_min = 0
    if tof_max is None:
        tof_max = 200000

    def _load_entry(entry, ws, title=""):
        # 1D plot
        ws_output = "%s %s" % (ws_output_base, title)
        if 1==0:
            ws_list.append(ws_output)
        else:
#            if not mtd.doesExist(ws):
            ws = LoadEventNexus(Filename=file_path,
                                NXentryName=entry)

            if ws.getNumberEvents()==0:
                print 'No data in entry %s' % entry
                return

            instr_dir = config.getInstrumentDirectory()

            # check date of input file
            date = mtd['ws'].getRun().getProperty('run_start').value
            nexus_acquisition = date.split('T')[0]
            if nexus_acquisition > '2014-10-01':
                geo_base_file_x = "REFL_Detector_Grouping_Sum_X_rot.xml"
                geo_base_file_y = "REFL_Detector_Grouping_Sum_Y_rot.xml"
            else:
                geo_base_file_x = "REFL_Detector_Grouping_Sum_X.xml"
                geo_base_file_y = "REFL_Detector_Grouping_Sum_Y.xml"

            if is_pixel_y:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             geo_base_file_x)
                GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output,
                               MapFile=grouping_file)
            else:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             geo_base_file_y)
                GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output,
                               MapFile=grouping_file)

            ws_output = Transpose(InputWorkspace=ws_output)

            # The Y pixel numbers start at 1 from the perspective of the users
            # They also read in reversed order
            if False and is_pixel_y:
                x=ws_output.dataX(0)
                y_reversed= ws_output.dataY(0)

                y=[i for i in y_reversed]
                for i in range(len(x)):
                    x[i] += 1
                    y_reversed[i] = y[len(y)-1-i]

            # Copy over the units
            ws_list.append(ws_output)

            # 2D plot
            output_2d = Rebin(InputWorkspace=ws,Params="%d,200,%d" % (tof_min, tof_max))

            if is_pixel_y:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             "REFL_Detector_Grouping_Sum_X.xml")
                output_2d = GroupDetectors(InputWorkspace=output_2d,\
                               MapFile=grouping_file)
            else:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             "REFL_Detector_Grouping_Sum_Y.xml")
                output_2d = GroupDetectors(InputWorkspace=output_2d,\
                               MapFile=grouping_file)

    if instrument=="REFM":
        for p in ['Off_Off', 'On_Off', 'Off_On', 'On_On']:
            ws = '%s - %s'%(ws_base,p)
            _load_entry("entry-%s" % p, ws, p)
    else:
        _load_entry("entry", ws_base)

    if callback is not None:
        from LargeScaleStructures import data_stitching
        data_stitching.RangeSelector.connect(ws_list, callback,
                                             range_min=range_min,
                                             range_max=range_max,
                                             x_title=x_title,
                                             log_scale=True,
                                             ws_output_base=ws_output_base)

    # Estimate peak limits
    ws_output = ws_base+'_all'
    CloneWorkspace(InputWorkspace=ws_list[0], OutputWorkspace=ws_output)
    for i in range(1,len(ws_list)):
        Plus(LHSWorkspace=ws_output, RHSWorkspace=ws_list[i],
             OutputWorkspace=ws_output)

    x = mtd[ws_output].readX(0)
    if not high_res:
        Rebin(InputWorkspace=ws_output, OutputWorkspace='__'+ws_output,
              Params="0,10,%d" % len(x))
        ws_output = '__'+ws_output
        x = mtd[ws_output].readX(0)
    y = mtd[ws_output].readY(0)

    n=[]
    slope_data=[]
    sum = 0.0
    min_id = 0
    max_id = 0
    max_slope = 0
    min_slope = 0
    for i in range(len(y)):
        sum += y[i]
        n.append(sum)
        if i>0:
            slope_data.append(y[i]-y[i-1])
            if slope_data[i-1]<min_slope:
                min_slope=slope_data[i-1]
                min_id = x[i-1]
            if slope_data[i-1]>max_slope:
                max_slope = slope_data[i-1]
                max_id = x[i-1]

    sigma = (min_id-max_id)/2.0
    mean = (min_id+max_id)/2.0
    return mean-2*sigma, mean+2*sigma

def get_logs(instrument, run):
    sangle = 0
    dangle = 0
    dangle0 = 0
    direct_beam_pix = 0
    det_distance = 2.562

    ws = "__%s_metadata" % run
    if instrument=="REFM":
        ws = '%s_%s'%(ws, 'Off_Off')

    if not mtd.workspaceExists(ws):
        try:
            f = FileFinder.findRuns("%s%s" % (instrument, run))[0]

            LoadEventNexus(Filename=f, OutputWorkspace=ws,
                           NXentryName='entry-Off_Off', MetaDataOnly=True)
        except:
            pass

    if mtd.workspaceExists(ws):
        if mtd[ws].getRun().hasProperty("SANGLE"):
            sangle = mtd[ws].getRun().getProperty("SANGLE").value[0]

        if mtd[ws].getRun().hasProperty("DANGLE"):
            dangle = mtd[ws].getRun().getProperty("DANGLE").value[0]

        if mtd[ws].getRun().hasProperty("DANGLE0"):
            dangle0 = mtd[ws].getRun().getProperty("DANGLE0").value[0]

        if mtd[ws].getRun().hasProperty("DIRPIX"):
            direct_beam_pix = mtd[ws].getRun().getProperty("DIRPIX").value[0]

        if mtd[ws].getRun().hasProperty("SampleDetDis"):
            det_distance = mtd[ws].getRun().getProperty("SampleDetDis").value[0]/1000.0

    return {"SANGLE":sangle,
            "DANGLE":dangle,
            "DANGLE0":dangle0,
            "DIRPIX":direct_beam_pix,
            "DET_DISTANCE":det_distance}

