import _qti
import os
from mantidsimple import *
from reduction.instruments.reflectometer import wks_utility

def tof_distribution(file_path, callback=None,
                     range_min=None, range_max=None):
    """
        Plot counts as a function of TOF for a given REF_L data file
    """
    basename = os.path.basename(file_path)
    ws_raw = "__%s" % basename
    ws = "__TOF_distribution"
    
    if not mtd.workspaceExists(ws_raw):
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
                                 high_res = True, instrument="REFL"):
    basename = os.path.basename(file_path)
    ws_base = "__%s" % basename
    ws_output_base = "Counts vs Y pixel"
    x_title = "Y pixel"
    if is_pixel_y is False:
        ws_output_base = "Counts vs X pixel"
        x_title = "X pixel"
        
    ws_list = []
    
    def _load_entry(entry, ws, title=""):
        if not mtd.workspaceExists(ws):
            LoadEventNexus(Filename=file_path, OutputWorkspace=ws,
                           NXentryName=entry)
            if mtd[ws].getNumberEvents()==0:
                mtd.deleteWorkspace(ws)
                return
            
        # 1D plot
        ws_output = "%s %s" % (ws_output_base, title)
        if is_pixel_y:
            GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output,
                           MapFile="Grouping/REFL_Detector_Grouping_Sum_X.xml")
        else:
            GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output,
                           MapFile="Grouping/REFL_Detector_Grouping_Sum_Y.xml")
            
        Transpose(InputWorkspace=ws_output, OutputWorkspace=ws_output)
        
        # The Y pixel numbers start at 1 from the perspective of the users
        # They also read in reversed order
        if is_pixel_y:
            x=mtd[ws_output].dataX(0)
            y_reversed=mtd[ws_output].dataY(0)
            y=[i for i in y_reversed]
            for i in range(len(x)):
                x[i] += 1
                y_reversed[i] = y[len(y)-1-i]
            
        # Copy over the units
        units = mtd[ws_output].getAxis(0).getUnit().name()
        mtd[ws_output].getAxis(0).setUnit(units)
        
        ws_list.append(ws_output)
            
        # 2D plot
        output_2d = ws_output+'_2D'
        Rebin(InputWorkspace=ws,OutputWorkspace=output_2d,Params="0,200,200000")
        if is_pixel_y:
            GroupDetectors(InputWorkspace=output_2d, OutputWorkspace=output_2d,
                           MapFile="Grouping/REFL_Detector_Grouping_Sum_X.xml")
        else:
            GroupDetectors(InputWorkspace=output_2d, OutputWorkspace=output_2d,
                           MapFile="Grouping/REFL_Detector_Grouping_Sum_Y.xml")
                
        
            
    if instrument=="REFM":
        for p in ['Off_Off', 'On_Off', 'Off_On', 'On_On']:
            ws = '%s_%s'%(ws_base,p)
            _load_entry("entry-%s" % p, ws, p)        
    else:
        _load_entry("entry", ws_base)
    
        
    if callback is not None:
        from LargeScaleStructures import data_stitching
        data_stitching.RangeSelector.connect(ws_list, callback,
                                             range_min=range_min,
                                             range_max=range_max,
                                             x_title=x_title)

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
    
    