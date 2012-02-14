import _qti
from mantidsimple import *
from reduction.instruments.reflectometer import wks_utility

def tof_distribution(file_path, callback=None):
    """
        Plot counts as a function of TOF for a given REF_L data file
    """
    ws = "__TOF_distribution"
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws)
    Rebin(InputWorkspace=ws,OutputWorkspace=ws,Params="0,200,200000")
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
        data_stitching.RangeSelector.connect([ws], callback, xmin=xmin, xmax=xmax)
    
def counts_vs_pixel_distribution(file_path, is_pixel_y=True, callback=None):
    ws = "__REF_data"
    ws_output = "Counts vs Y pixel"
    if is_pixel_y is False:
        ws_output = "Counts vs X pixel"
        
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws)
    
    # 1D plot
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
    
    # 2D plot
    Rebin(InputWorkspace=ws,OutputWorkspace=ws,Params="0,200,200000")
    if is_pixel_y:
        GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output+'_2D',
                       MapFile="Grouping/REFL_Detector_Grouping_Sum_X.xml")
    else:
        GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output+'_2D',
                       MapFile="Grouping/REFL_Detector_Grouping_Sum_Y.xml")
    
    if callback is not None:
        from LargeScaleStructures import data_stitching
        data_stitching.RangeSelector.connect([ws_output], callback)

    