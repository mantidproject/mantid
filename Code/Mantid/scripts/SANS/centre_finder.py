import isis_reducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
from mantidsimple import *
import SANSUtility

class CentreFinder(object):
    """
        Aids estimating the effective centre of the particle beam by calculating Q in four
        quadrants and using the asymmetry to calculate the direction to the beam centre. A
        better estimate for the beam centre position can hence be calculated iteratively 
    """  
    QUADS = ['Left', 'Right', 'Up', 'Down']
    def __init__(self, guess_centre):
        """
            Takes a loaded reducer (sample information etc.) and the initial guess of the centre
            position that are required for all later iterations
            @param guess_centre: the starting position that the trial x and y are relative to
        """
        self._last_pos = guess_centre
        self.detector = None

    def SeekCentre(self, setup, trial):
        """
            Does four calculations of Q to estimate a better centre location than the one passed
            to it
            @param setup: the reduction chain object that contains information about the reduction
            @param trial: the coordinates of the location to test as a list in the form [x, y]
            @return: the asymmetry in the calculated Q in the x and y directions  
        """
        
        self.detector = setup.instrument.cur_detector().name()
    
        self.move(setup, trial[0]-self._last_pos[0], trial[1]-self._last_pos[1])
    
        #phi masking will remove areas of the detector that we need 
        setup.mask.mask_phi = False
        
        setup.pre_process()
        setup.output_wksp = 'centre'
        steps = setup._conv_Q
        steps = steps[0:len(steps)-1]
        setup._reduce(init=False, post=False, steps=steps)
    
        self._group_into_quadrants(setup, 'centre', trial[0], trial[1], suffix='_tmp')
    
        if setup.background_subtracter:
            #reduce the can here
            setup.reduce_can('centre_can', run_Q=False)
            
            self._group_into_quadrants(setup, 'centre_can', trial[0], trial[1], suffix='_can')
            Minus('Left_tmp', 'Left_can', 'Left_tmp')
            Minus('Right_tmp', 'Right_can', 'Right_tmp')
            Minus('Up_tmp', 'Up_can', 'Up_tmp')
            Minus('Down_tmp', 'Down_can', 'Down_tmp')    
            DeleteWorkspace('Left_can')
            DeleteWorkspace('Right_can')
            DeleteWorkspace('Up_can')
            DeleteWorkspace('Down_can')
            DeleteWorkspace('centre_can')
        
        DeleteWorkspace('centre')
        self._last_pos = trial
        
        #prepare the workspaces for "publication", after they have their standard names calculations will be done on them and they will be plotted
        for out_wksp in self.QUADS:
            in_wksp = out_wksp+'_tmp' 
            ReplaceSpecialValues(InputWorkspace=in_wksp,OutputWorkspace=in_wksp,NaNValue=0,InfinityValue=0)
            rem_zeros = sans_reduction_steps.StripEndZeros()
            rem_zeros.execute(setup, in_wksp)
    
            RenameWorkspace(in_wksp, out_wksp)
    
        return self._calculate_residue()                        
    
    def status_str(self, iter, x_res, y_res):
        """
            Creates a human readble string from the numbers passed to it
            @param iter: iteration number
            @param x_res: asymmetry in the x direction
            @param y_res: asymmetry in y
            @return: a human readable string
        """ 
        x_str = str(self._last_pos[0]*1000.).ljust(10)[0:9]
        y_str = str(self._last_pos[1]*1000.).ljust(10)[0:9]
        x_res = '    SX='+str(x_res).ljust(7)[0:6]
        y_res = '    SY='+str(y_res).ljust(7)[0:6]
        return '::SANS::Itr '+str(iter)+':  ('+x_str+',  '+y_str+')'+x_res+y_res
    
    def move(self, setup, x, y):
        """
            Move the selected detector in both the can and sample workspaces, remembering the
            that ISIS SANS team see the detector from the other side
            @param setup: the reduction chain object that contains information about the reduction
            @param x: the distance to move in the x (-x) direction in metres
            @param y: the distance to move in the y (-y) direction in metres
        """
        x = -x
        y = -y
        MoveInstrumentComponent(setup.get_sample().wksp_name,
            ComponentName=self.detector, X=x, Y=y, RelativePosition=True)
        if setup.background_subtracter:
            MoveInstrumentComponent(setup.background_subtracter.workspace.wksp_name,
                ComponentName=self.detector, X=x, Y=y, RelativePosition=True)

    # Create a workspace with a quadrant value in it 
    def _create_quadrant(self, setup, reduced_ws, quadrant, xcentre, ycentre, r_min, r_max, suffix):
        out_ws = quadrant+suffix
        # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
        CloneWorkspace(reduced_ws, out_ws)
        objxml = SANSUtility.QuadrantXML([0, 0, 0.0], r_min, r_max, quadrant)
        # Mask out everything outside the quadrant of interest
        MaskDetectorsInShape(out_ws, objxml)
    
        setup.to_Q.execute(setup, out_ws)
        #Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)
    
    # Create 4 quadrants for the centre finding algorithm and return their names
    def _group_into_quadrants(self, setup, input, xcentre, ycentre, suffix=''):
        r_min = setup.CENT_FIND_RMIN
        r_max = setup.CENT_FIND_RMAX
    
        for q in self.QUADS:
            self._create_quadrant(setup, input, q, xcentre, ycentre, r_min, r_max, suffix)
    
    def _calculate_residue(self):
        """
            Calculate the sum squared difference between pairs of workspaces named Left, Right, Up
            and Down. This assumes that a workspace with one spectrum for each of the quadrants
            @return: difference left to right, difference up down 
        """
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
