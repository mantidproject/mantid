import isis_reducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
from mantidsimple import *
import SANSUtility

class CentreFinder():
    """
        Aids estimating the effective centre of the particle beam by calculating Q in four
        quadrants and using the asymmetry to calculate the direction to the beam centre. A
        better estimate for the beam centre position can hence be calculated iteratively 
    """  
    QUADS = ['Left', 'Right', 'Up', 'Down']
    def __init__(self, setup, guess_centre):
        """
            Takes a loaded reducer (sample information etc.) and the initial guess of the centre
            position that are required for all later iterations
            @param setup: the reduction chain object that contains information about the reduction
            @param guess_centre: the starting position that the trial x and y are relative to
        """
        self.reducer = setup
        self._last_pos = guess_centre

    def SeekCentre(self,trial):
        """
            Does four calculations of Q to estimate a better centre location than the one passed
            to it
            @param trial: the coordinates of the location to test as a list in the form [x, y]
            @return: the asymmetry in the calculated Q in the x and y directions  
        """
        
        currentDet = self.reducer.instrument.cur_detector().name()
    
        MoveInstrumentComponent(self.reducer.sample_wksp,
            ComponentName=currentDet, X=trial[0]-self._last_pos[0],
            Y=trial[1]-self._last_pos[1], RelativePosition=True)
    
        #phi masking will remove areas of the detector that we need 
        self.reducer.mask.mask_phi = False
        self.reducer.keep_un_normalised(False)
        self.reducer.run_no_Q('centre')
    
        self._group_into_quadrants('centre', trial[0], trial[1], suffix='_tmp')
    
        if self.reducer.background_subtracter:
            MoveInstrumentComponent(self.reducer.background_subtracter.workspace.wksp_name,
                ComponentName=currentDet, X=trial[0]-self._last_pos[0],
                Y=trial[1]-self._last_pos[1], RelativePosition=True)
            
            #reduce the can here
            self.reducer.reduce_can(self.reducer.background_subtracter.workspace.wksp_name, 'centre_can', run_Q=False)
            
            self._group_into_quadrants('centre_can', trial[0], trial[1], suffix='_can')
            Minus('Left_tmp', 'Left_can', 'Left_tmp')
            Minus('Right_tmp', 'Right_can', 'Right_tmp')
            Minus('Up_tmp', 'Up_can', 'Up_tmp')
            Minus('Down_tmp', 'Down_can', 'Down_tmp')    
            DeleteWorkspace('Left_can')
            DeleteWorkspace('Right_can')
            DeleteWorkspace('Up_can')
            DeleteWorkspace('Down_can')
        
        self._last_pos = trial
        
        #prepare the workspaces for "publication", after they have their standard names calculations will be done on them and they will be plotted
        for out_wksp in self.QUADS:
            in_wksp = out_wksp+'_tmp' 
            ReplaceSpecialValues(InputWorkspace=in_wksp,OutputWorkspace=in_wksp,NaNValue=0,InfinityValue=0)
            rem_zeros = sans_reduction_steps.StripEndZeros()
            rem_zeros.execute(self.reducer, in_wksp)
    
            RenameWorkspace(in_wksp, out_wksp)
    
        return self._calculate_residue()                        
    
    def status_str(self, iter, x, y, x_res, y_res):
        """
            Creates a human readble string from the numbers passed to it
            @param iter: iteration number
            @param x: current x-coordinate
            @param y: current y-coordinate
            @param x_res: asymmetry in the x direction
            @param y_res: asymmetry in y
            @return: a human readable string
        """ 
        x_str = str(x*1000.).ljust(10)[0:9]
        y_str = str(y*1000.).ljust(10)[0:9]
        x_res = '    SX='+str(x_res).ljust(7)[0:6]
        y_res = '    SY='+str(y_res).ljust(7)[0:6]
        return '::SANS::Itr '+str(iter)+':  ('+x_str+',  '+y_str+')'+x_res+y_res
            
    # Create a workspace with a quadrant value in it 
    def _create_quadrant(self, reduced_ws, quadrant, xcentre, ycentre, r_min, r_max, suffix):
        out_ws = quadrant+suffix
        # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
        CloneWorkspace(reduced_ws, out_ws)
        objxml = SANSUtility.QuadrantXML([0, 0, 0.0], r_min, r_max, quadrant)
        # Mask out everything outside the quadrant of interest
        MaskDetectorsInShape(out_ws,objxml)
    
        # Q1D ignores masked spectra/detectors. This is on the InputWorkspace, so we don't need masking of the InputForErrors workspace
    
        self.reducer.to_Q.execute(self.reducer, out_ws)
        #Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)
    
    # Create 4 quadrants for the centre finding algorithm and return their names
    def _group_into_quadrants(self, input, xcentre, ycentre, suffix=''):
        r_min = self.reducer.CENT_FIND_RMIN
        r_max = self.reducer.CENT_FIND_RMAX
    
        for q in self.QUADS:
            self._create_quadrant(input, q, xcentre, ycentre, r_min, r_max, suffix)
    
        # We don't need these now
    #    mantid.deleteWorkspace(input)
    
    # Calcluate the sum squared difference of the given workspaces. This assumes that a workspace with
    # one spectrum for each of the quadrants. The order should be L,R,U,D.
    def _calculate_residue(self):
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
