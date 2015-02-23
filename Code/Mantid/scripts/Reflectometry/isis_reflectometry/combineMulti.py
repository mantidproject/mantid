#pylint: disable=invalid-name
from l2q import *
from mantid.simpleapi import *
import math
from mantid.api import WorkspaceGroup

def combineDataMulti(wksp_list,output_wksp,beg_overlap,end_overlap,Qmin,Qmax,binning,scale_high=1,scale_factor=-1.0,which_period=1,keep=0):
    """
    Function stitches multiple workspaces together. Workspaces should have an X-axis in mod Q, and the Spectrum axis as I/I0

    wksp_list: A list of workspaces to stitch together
    ouput_wksp: output workspace name
    beg_overlap: The beginning of the overlap region. List argument, each entry matches the entry in the wksp_list
    end_overlap: The end of the overlap region. List argument, each entry matches the entry in the wksp_list
    Qmin: Q minimum of the final output workspace
    Qmax: Q maximum of the input workspace
    which_period: Which period to use if multiperiod workspaces are provided.
    keep=1: keep individual workspaces in Mantid, otherwise delete wksp_list
    """

    # check if overlaps have correct number of entries
    defaultoverlaps = False
    if type(beg_overlap) != list:
        beg_overlap = [beg_overlap]
    if type(end_overlap) != list:
        end_overlap = [end_overlap]
    if len(wksp_list) != len(beg_overlap):
        print "Using default values!"
        defaultoverlaps = True

    #copy first workspace into temporary wksp 'currentSum'
    currentSum = CloneWorkspace(InputWorkspace=wksp_list[0])
    print "Length: ",len(wksp_list), wksp_list

    for i in range(0,len(wksp_list)-1):
        w1=currentSum
        w2=getWorkspace(wksp_list[i+1]) # TODO: distinguishing between a group and a individual workspace is unnecessary for an algorithm. But custom group behavior WILL be required.
        if defaultoverlaps:
            overlapLow = w2.readX(0)[0]
            overlapHigh = 0.5*max(w1.readX(0))
        else:
            overlapLow = beg_overlap[i+1]
            overlapHigh = end_overlap[i]
        print "Iteration",i
        currentSum, scale_factor = stitch2(currentSum, mtd[wksp_list[i+1]], currentSum.name(), overlapLow, overlapHigh, Qmin, Qmax, binning, scale_high)
    RenameWorkspace(InputWorkspace=currentSum.name(),OutputWorkspace=output_wksp)

    # Remove any existing workspaces from the workspace list.
    if not keep:
        names = mtd.getObjectNames()
        for ws in wksp_list:
            candidate = ws
            if candidate in names:
                DeleteWorkspace(candidate)

    return mtd[output_wksp]

def stitch2(ws1, ws2, output_ws_name, begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=True,scalefactor=-1.0):
    """
    Function stitches two workspaces together and returns a stitched workspace along with the scale factor

    ws1: First workspace to stitch
    ws2: Second workspace to stitch
    output_ws_name: The name to give the outputworkspace
    begoverlap: The beginning of the overlap region
    endoverlap: The end of the overlap region
    Qmin: Final minimum Q in the Q range
    Qmax: Final maximum Q in the Q range
    binning: Binning stride to use
    scalehigh: if True, scale ws2, otherwise scale ws1
    scalefactor: Use the manual scaling factor provided if > 0
    """
    if scalefactor > 0.0:
        manual_scalefactor = True
    else:
        manual_scalefactor = False
        scalefactor = 1.0
    # Interally use the Stitch1D algorithm.
    outputs = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2,\
    		OutputWorkspace=output_ws_name, StartOverlap=begoverlap, EndOverlap=endoverlap,\
    		UseManualScaleFactor=manual_scalefactor,\
    		ManualScaleFactor=scalefactor, Params="%f,%f,%f" % (Qmin, binning, Qmax))

    return outputs


def combine2(wksp1,wksp2,outputwksp,begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=True,scalefactor=-1.0):
    """
    Function stitches two workspaces together and returns a stitched workspace name along with the scale factor

    wksp1: First workspace name to stitch
    wksp2: Second workspace name to stitch
    outputwksp: The name to give the outputworkspace
    begoverlap: The beginning of the overlap region
    endoverlap: The end of the overlap region
    Qmin: Final minimum Q in the Q range
    Qmax: Final maximum Q in the Q range
    binning: Binning stride to use
    scalehigh: if True, scale ws2, otherwise scale ws1
    scalefactor: Use the manual scaling factor provided if > 0
    """
    if scalefactor > 0.0:
        manual_scalefactor = True
    else:
        manual_scalefactor = False
        scalefactor = 1.0
    # Interally use the Stitch1D algorithm.
    outputs = Stitch1D(LHSWorkspace=mtd[wksp1], RHSWorkspace=mtd[wksp2],\
    		OutputWorkspace=outputwksp, StartOverlap=begoverlap, EndOverlap=endoverlap,\
    		UseManualScaleFactor=manual_scalefactor,\
    		ManualScaleFactor=scalefactor, Params="%f,%f,%f" % (Qmin, binning, Qmax))

    outscalefactor = outputs[1]

    return (outputwksp, outscalefactor)

def getWorkspace(wksp):
    """
    Get the workspace if it is not a group workspace. If it is a group workspace, get the first period.
    """
    if isinstance(mtd[wksp], WorkspaceGroup):
        wout = mtd[wksp][0]
    else:
        wout = mtd[wksp]
    return wout



