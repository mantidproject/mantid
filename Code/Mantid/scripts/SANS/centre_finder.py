import isis_reducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
from mantidsimple import *
import SANSUtility

_PIECES = ['Left', 'Right', 'Up', 'Down']

def SeekCentre(trial, reducer, origin):
    """
        Does four calculations of Q to estimate a better centre location than the one passed
        to it
        @param trial: the coordinates of the location to test as a list in the form [x, y]
        @param reducer: the reduction chain object that contains information about the reduction
        @param origin: the starting position that the trial x and y are relative to
        @return: the coordinates of a location closer to the centre
    """
    
    currentDet = reducer.instrument.cur_detector().name()

    MoveInstrumentComponent(reducer.sample_wksp,
        ComponentName=currentDet, X=trial[0]-origin[0], Y=trial[1]-origin[1], RelativePosition=True)

    #phi masking will remove areas of the detector that we need 
    reducer.mask.mask_phi = False
    reducer.keep_un_normalised(False)
    reducer.run_no_Q('centre')

    GroupIntoQuadrants('centre', trial[0], trial[1], reducer, suffix='_tmp')

    if reducer.background_subtracter:
        MoveInstrumentComponent(reducer.background_subtracter.workspace.wksp_name,
            ComponentName=currentDet, X=trial[0]-origin[0], Y=trial[1]-origin[1], RelativePosition=True)
        
        #reduce the can here
        reducer.reduce_can(reducer.background_subtracter.workspace.wksp_name, 'centre_can', run_Q=False)
        
        GroupIntoQuadrants('centre_can', trial[0], trial[1], reducer, suffix='_can')
        Minus('Left_tmp', 'Left_can', 'Left_tmp')
        Minus('Right_tmp', 'Right_can', 'Right_tmp')
        Minus('Up_tmp', 'Up_can', 'Up_tmp')
        Minus('Down_tmp', 'Down_can', 'Down_tmp')    
        DeleteWorkspace('Left_can')
        DeleteWorkspace('Right_can')
        DeleteWorkspace('Up_can')
        DeleteWorkspace('Down_can')
    
    #prepare the workspaces for "publication", after they have their standard names calculations will be done on them and they will be plotted
    for out_wksp in _PIECES:
        in_wksp = out_wksp+'_tmp' 
        ReplaceSpecialValues(InputWorkspace=in_wksp,OutputWorkspace=in_wksp,NaNValue=0,InfinityValue=0)
        rem_zeros = sans_reduction_steps.StripEndZeros()
        rem_zeros.execute(reducer, in_wksp)

        RenameWorkspace(in_wksp, out_wksp)

    return CalculateResidue()                        
    
# Create a workspace with a quadrant value in it 
def CreateQuadrant(reduced_ws, quadrant, xcentre, ycentre, reducer, r_min, r_max, suffix):
    out_ws = quadrant+suffix
    # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
    CloneWorkspace(reduced_ws, out_ws)
    objxml = SANSUtility.QuadrantXML([0, 0, 0.0], r_min, r_max, quadrant)
    # Mask out everything outside the quadrant of interest
    MaskDetectorsInShape(out_ws,objxml)

    # Q1D ignores masked spectra/detectors. This is on the InputWorkspace, so we don't need masking of the InputForErrors workspace

    reducer.to_Q.execute(reducer, out_ws)
    #Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)

# Create 4 quadrants for the centre finding algorithm and return their names
def GroupIntoQuadrants(input, xcentre, ycentre, reducer, suffix=''):
    r_min = reducer.CENT_FIND_RMIN
    r_max = reducer.CENT_FIND_RMAX

    for q in _PIECES:
        CreateQuadrant(input, q, xcentre, ycentre, reducer, r_min, r_max, suffix)

    # We don't need these now
#    mantid.deleteWorkspace(input)

# Calcluate the sum squared difference of the given workspaces. This assumes that a workspace with
# one spectrum for each of the quadrants. The order should be L,R,U,D.
def CalculateResidue():
    yvalsA = mtd.getMatrixWorkspace('Left').readY(0)
    yvalsB = mtd.getMatrixWorkspace('Right').readY(0)
    qvalsA = mtd.getMatrixWorkspace('Left').readX(0)
    qvalsB = mtd.getMatrixWorkspace('Right').readX(0)
    qrange = [len(yvalsA), len(yvalsB)]
    nvals = min(qrange)
    residueX = 0
    indexB = 0
    for indexA in range(0, nvals):
        if qvalsA[indexA] < qvalsB[indexB]:
            mantid.sendLogMessage("::SANS::LR1 "+str(indexA)+" "+str(indexB))
            continue
        elif qvalsA[indexA] > qvalsB[indexB]:
            while qvalsA[indexA] > qvalsB[indexB]:
                mantid.sendLogMessage("::SANS::LR2 "+str(indexA)+" "+str(indexB))
                indexB += 1
        if indexA > nvals - 1 or indexB > nvals - 1:
            break
        residueX += pow(yvalsA[indexA] - yvalsB[indexB], 2)
        indexB += 1

    yvalsA = mtd.getMatrixWorkspace('Up').readY(0)
    yvalsB = mtd.getMatrixWorkspace('Down').readY(0)
    qvalsA = mtd.getMatrixWorkspace('Up').readX(0)
    qvalsB = mtd.getMatrixWorkspace('Down').readX(0)
    qrange = [len(yvalsA), len(yvalsB)]
    nvals = min(qrange)
    residueY = 0
    indexB = 0
    for indexA in range(0, nvals):
        if qvalsA[indexA] < qvalsB[indexB]:
            mantid.sendLogMessage("::SANS::UD1 "+str(indexA)+" "+str(indexB))
            continue
        elif qvalsA[indexA] > qvalsB[indexB]:
            while qvalsA[indexA] > qvalsB[indexB]:
                mantid.sendLogMessage("::SANS::UD2 "+str(indexA)+" "+str(indexB))
                indexB += 1
        if indexA > nvals - 1 or indexB > nvals - 1:
            break
        residueY += pow(yvalsA[indexA] - yvalsB[indexB], 2)
        indexB += 1
  
    return residueX, residueY
