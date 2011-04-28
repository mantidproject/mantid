import isis_reducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
from mantidsimple import *
import SANSUtility



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

    reducer.output_wksp = reducer.sample_wksp
    reducer.keep_un_normalised(False)
    reducer.run_no_Q('centre')

    GroupIntoQuadrants('centre', trial[0], trial[1], reducer)

    if reducer.background_subtracter:
        MoveInstrumentComponent(reducer.background_subtracter.workspace.wksp_name,
            ComponentName=currentDet, X=trial[0]-origin[0], Y=trial[1]-origin[1], RelativePosition=True)
        
        #reduce the can here
        reducer.reduce_can(reducer.background_subtracter.workspace.wksp_name, 'centre_can', run_Q=False)
        
        RenameWorkspace('Left', 'Left_sample')
        RenameWorkspace('Right', 'Right_sample')
        RenameWorkspace('Up', 'Up_sample')
        RenameWorkspace('Down', 'Down_sample')
        GroupIntoQuadrants('centre_can', trial[0], trial[1], reducer)
        Minus('Left_sample', 'Left', 'Left')
        Minus('Right_sample', 'Right', 'Right')
        Minus('Up_sample', 'Up', 'Up')
        Minus('Down_sample', 'Down', 'Down')
    
    return CalculateResidue()                        
    
# Create a workspace with a quadrant value in it 
def CreateQuadrant(reduced_ws, quadrant, xcentre, ycentre, reducer, r_min, r_max):
    # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
    CloneWorkspace(reduced_ws, quadrant)
    objxml = SANSUtility.QuadrantXML([0, 0, 0.0], r_min, r_max, quadrant)
    # Mask out everything outside the quadrant of interest
    MaskDetectorsInShape(quadrant,objxml)

    # Q1D ignores masked spectra/detectors. This is on the InputWorkspace, so we don't need masking of the InputForErrors workspace

    reducer.to_Q.execute(reducer, quadrant)
    #Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)

    flag_value = -10.0
    ReplaceSpecialValues(InputWorkspace=quadrant,OutputWorkspace=quadrant,NaNValue=flag_value,InfinityValue=flag_value)

    rem_zeros = sans_reduction_steps.StripEndZeros()
    rem_zeros.execute(reducer, quadrant)

# Create 4 quadrants for the centre finding algorithm and return their names
def GroupIntoQuadrants(input, xcentre, ycentre, reducer):
    r_min = reducer.CENT_FIND_RMIN
    r_max = reducer.CENT_FIND_RMAX

    pieces = ['Left', 'Right', 'Up', 'Down']
    for q in pieces:
        CreateQuadrant(input, q, xcentre, ycentre, reducer, r_min, r_max)

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
