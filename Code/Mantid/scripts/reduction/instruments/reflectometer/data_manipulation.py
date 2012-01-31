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
    
def counts_vs_y_distribution(file_path, minTOF, maxTOF):
    ws = "__REFL_data"
    ws_output = "__REFL_Y_distribution"
    graph_name = "Counts vs Y"
    LoadEventNexus(Filename=file_path, OutputWorkspace=ws)
    ws_integrated = wks_utility.createIntegratedWorkspace(mtd[ws],ws_output,0,303,0,255)
    Transpose(InputWorkspace=ws_output, OutputWorkspace=ws_output)

    Rebin(InputWorkspace=ws,OutputWorkspace=ws,Params="0,200,200000")
    ws_integrated = wks_utility.createIntegratedWorkspace(mtd[ws],ws_output+"_2D",0,303,0,255)
    
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
        
    