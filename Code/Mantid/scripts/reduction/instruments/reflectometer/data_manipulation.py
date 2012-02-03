import _qti
from mantidsimple import *
from reduction.instruments.reflectometer import wks_utility

def tof_distribution(file_path):
    """
        Plot counts as a function of TOF for a given REF_L data file
    """
    ws = "__REFL_TOF_distribution"
    graph_name = "TOF distribution"
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws)
    Rebin(InputWorkspace=ws,OutputWorkspace=ws,Params="0,200,200000")
    SumSpectra(InputWorkspace=ws, OutputWorkspace=ws)
    g = _qti.app.graph(graph_name)
    if g is None:
        g = _qti.app.mantidUI.pyPlotSpectraList([ws],[0],True)
        g.setName(graph_name)
        x = mtd[ws].readX(0)
        y = mtd[ws].readY(0)
        xmin = x[0]
        xmax = None
        for i in range(len(y)):
            if y[i]==0.0 and xmax is None:
                xmin = x[i]
            if y[i]>0:
                xmax = x[i]
                
        l=g.activeLayer()
        l.setScale(2,xmin,xmax)
        l.setTitle(" ")
    
def counts_vs_y_distribution(file_path, minTOF, maxTOF, callback=None):
    ws = "__REFL_data"
    ws_output = "__REFL_Y_distribution"
    graph_name = "Counts vs Y"
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws)
    
    # 1D plot
    GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output,
                   MapFile="Grouping/REFL_Detector_Grouping_Sum_X.xml")
    Transpose(InputWorkspace=ws_output, OutputWorkspace=ws_output)
    
    # The pixel numbers start at 1 from the perspective of the users
    # They also read in reversed order
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
    GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws_output+'_2D',
                   MapFile="Grouping/REFL_Detector_Grouping_Sum_X.xml")
    
    if callback is not None:
        from LargeScaleStructures import data_stitching
        #def callback(xmin,xmax):
        #    print xmin,xmax
        data_stitching.RangeSelector.connect([ws_output], callback)

    return

    g = _qti.app.graph(graph_name)
    if g is None:
        g = _qti.app.mantidUI.pyPlotSpectraList([ws_output],[0],True)
        g.setName(graph_name)  
        x = mtd[ws_output].readX(0)
        y = mtd[ws_output].readY(0)
        xmin = x[0]
        xmax = None
        for i in range(len(y)):
            if y[i]==0.0 and xmax is None:
                xmin = x[i]
            if y[i]>0:
                xmax = x[i]
                
        l=g.activeLayer()
        l.setScale(2,xmin,xmax)
        l.setTitle(" ")
        
    