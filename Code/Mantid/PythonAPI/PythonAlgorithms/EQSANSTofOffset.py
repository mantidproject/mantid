from MantidFramework import *
from mantidsimple import *

class EQSANSTofOffset(PythonAlgorithm):
    """
        Calculate TOF offset to be added to the TOF binning of EQSANS data files
    """
    
    # Source repetition rate (Hz)
    REP_RATE = 60.0
    # Pulse widge (micro sec per angstrom)
    PULSEWIDTH = 20.0
    # Chopper phase offset (micro sec)
    CHOPPER_PHASE_OFFSET = [[9507.,9471.,9829.7,9584.3],[19024.,18820.,19714.,19360.]]
    # Chopper angles (degree)
    CHOPPER_ANGLE = [129.605,179.989,230.010,230.007]
    # Chopper location (mm)
    CHOPPER_LOCATION = [5700.,7800.,9497.,9507.]
    
    def category(self):
        return "SANS"

    def name(self):
        return "EQSANSTofOffset"

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input)
        self.declareProperty("FrameSkipping", False)
        self.declareProperty("Offset", 0.0, Direction=Direction.Output)

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace")
        frame_skipping = self.getProperty("FrameSkipping")

        # Storage for chopper information read from the logs
        chopper_set_phase = 4*[0]
        chopper_speed = 4*[0]
        chopper_actual_phase = 4*[0]
        chopper_wl_1 = 4*[0]
        chopper_wl_2 = 4*[0]
        frame_wl_1 = 0
        frame_srcpulse_wl_1 = 0
        frame_wl_2 = 0
        chopper_srcpulse_wl_1 = 4*[0]
        chopper_frameskip_wl_1 = 4*[0]
        chopper_frameskip_wl_2 = 4*[0]
        chopper_frameskip_srcpulse_wl_1 = 4*[0]
        
        tof_frame_width = 1.0e6/self.REP_RATE;

        tmp_frame_width = tof_frame_width
        if frame_skipping:
            tmp_frame_width *= 2.0
            
        # Choice of parameter set 
        m_set = 0
        if frame_skipping:
            m_set = 1
    
        first=True
        first_skip=True
    
        for i in range(4):
            # Read chopper information
            log_str = 'Phase'+str(i+1)
            chopper_set_phase[i] = input_ws.getRun()[log_str].getStatistics().mean
            log_str = 'Speed'+str(i+1)
            chopper_speed[i] = input_ws.getRun()[log_str].getStatistics().mean

            # Only process choppers with non-zero speed
            if chopper_speed[i]<=0:
                continue
            
            chopper_actual_phase[i] = chopper_set_phase[i] - self.CHOPPER_PHASE_OFFSET[m_set][i]
            
            while chopper_actual_phase[i]<0:
                chopper_actual_phase[i] += tmp_frame_width
    
            x1 = ( chopper_actual_phase[i]- ( tmp_frame_width * 0.5*self.CHOPPER_ANGLE[i]/360. ) ) # opening edge
            x2 = ( chopper_actual_phase[i]+ ( tmp_frame_width * 0.5*self.CHOPPER_ANGLE[i]/360. ) ) # closing edge
            if not frame_skipping: # not skipping
                while x1<0: 
                    x1+=tmp_frame_width
                    x2+=tmp_frame_width
            
            if x1>0:
                chopper_wl_1[i]= 3.9560346 * x1 /self.CHOPPER_LOCATION[i]
                #if i==3:
                    #print "offset", 12.*self.CHOPPER_LOCATION[i]/3.9560346+( tmp_frame_width * 0.5*self.CHOPPER_ANGLE[i]/360. )-chopper_set_phase[i]
                chopper_srcpulse_wl_1[i]= 3.9560346 * ( x1-chopper_wl_1[i]*self.PULSEWIDTH ) /self.CHOPPER_LOCATION[i]
            else:
                chopper_wl_1[i]=chopper_srcpulse_wl_1[i]=0.
            
            if x2>0: 
                chopper_wl_2[i]= 3.9560346 * x2 /self.CHOPPER_LOCATION[i]
            else:
                chopper_wl_2[i]=0.

            if first==True:
                frame_wl_1=chopper_wl_1[i]
                frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i]
                frame_wl_2=chopper_wl_2[i]
                first=False
            else:
                if frame_skipping==True and i==2:    # ignore chopper 1 and 2 forthe shortest wl.
                    frame_wl_1=chopper_wl_1[i]
                    frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i]
                if frame_wl_1<chopper_wl_1[i]:
                    frame_wl_1=chopper_wl_1[i]
                if frame_wl_2>chopper_wl_2[i]:
                    frame_wl_2=chopper_wl_2[i]
                if frame_srcpulse_wl_1<chopper_srcpulse_wl_1[i]:
                    frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i]

            if frame_skipping==True:
                if x1>0:
                    x1 += tof_frame_width;    # skipped pulse
                    chopper_frameskip_wl_1[i]= 3.9560346 * x1 /self.CHOPPER_LOCATION[i]
                    chopper_frameskip_srcpulse_wl_1[i]= 3.9560346 * ( x1-chopper_wl_1[i]*self.PULSEWIDTH ) /self.CHOPPER_LOCATION[i]
                else:
                    chopper_wl_1[i]=chopper_srcpulse_wl_1[i]=0.

                if x2>0:
                    x2 += tof_frame_width
                    chopper_frameskip_wl_2[i]= 3.9560346 * x2 /self.CHOPPER_LOCATION[i]
                else:
                    chopper_wl_2[i]=0.

                if i<2 and chopper_frameskip_wl_1[i] > chopper_frameskip_wl_2[i]:
                     continue
                 
                if first_skip:
                    frameskip_wl_1=chopper_frameskip_wl_1[i]
                    frameskip_srcpulse_wl_1=chopper_frameskip_srcpulse_wl_1[i]
                    frameskip_wl_2=chopper_frameskip_wl_2[i]
                    first_skip=False
                else:
                    if i==2:   # ignore chopper 1 and 2 forthe longest wl.
                        frameskip_wl_2=chopper_frameskip_wl_2[i]
                    
                    if chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] and frameskip_wl_1<chopper_frameskip_wl_1[i]:
                        frameskip_wl_1=chopper_frameskip_wl_1[i]
                    if chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] and frameskip_srcpulse_wl_1<chopper_frameskip_srcpulse_wl_1[i]:
                        frameskip_srcpulse_wl_1=chopper_frameskip_srcpulse_wl_1[i]

                    if frameskip_wl_2>chopper_frameskip_wl_2[i]:
                        frameskip_wl_2=chopper_frameskip_wl_2[i]
    
        if frame_wl_1>=frame_wl_2:    # too many frames later. So figure it out
            n_frame = 4*[0]
            c_wl_1 = 4*[0]
            c_wl_2 = 4*[0]
            passed=False;
            
            while not passed and n_frame[0]<99:
                frame_wl_1=c_wl_1[0] = chopper_wl_1[0] + 3.9560346 * n_frame[0] * tof_frame_width /self.CHOPPER_LOCATION[0];
                frame_wl_2=c_wl_2[0] = chopper_wl_2[0] + 3.9560346 * n_frame[0] * tof_frame_width /self.CHOPPER_LOCATION[0];
    
                for i in range(4):
                    n_frame[i] = n_frame[i-1] - 1
                    passed=False
                    
                    while n_frame[i] - n_frame[i-1] < 10:
                        n_frame[i] += 1
                        c_wl_1[i] = chopper_wl_1[i] + 3.9560346 * n_frame[i] * tof_frame_width /self.CHOPPER_LOCATION[i];
                        c_wl_2[i] = chopper_wl_2[i] + 3.9560346 * n_frame[i] * tof_frame_width /self.CHOPPER_LOCATION[i];
    
                        if frame_wl_1 < c_wl_2[i] and frame_wl_2> c_wl_1[i]:
                            passed=True
                            break
                        if frame_wl_2 < c_wl_1[i]:
                            break # over shot
    
                    if not passed:
                        n_frame[0] += 1
                        break
                    else:
                        if frame_wl_1<c_wl_1[i]: 
                            frame_wl_1=c_wl_1[i]
                        if frame_wl_2>c_wl_2[i]: 
                             frame_wl_2=c_wl_2[i]

            if frame_wl_2 > frame_wl_1:
                if c_wl_1[2] > c_wl_1[3]:
                    n = 2
                else: 
                    n = 3
                frame_srcpulse_wl_1=c_wl_1[n] - 3.9560346 * c_wl_1[n] * self.PULSEWIDTH /self.CHOPPER_LOCATION[n];
    
                for i in range(4):
                    chopper_wl_1[i] = c_wl_1[i]
                    chopper_wl_2[i] = c_wl_2[i]
                    if frame_skipping==True:
                        
                        chopper_frameskip_wl_1[i] = c_wl_1[i] +  3.9560346 * 2.* tof_frame_width /self.CHOPPER_LOCATION[i];
                        chopper_frameskip_wl_2[i] = c_wl_2[i] +  3.9560346 * 2.* tof_frame_width /self.CHOPPER_LOCATION[i];
                        if i==0:
                            frameskip_wl_1 = chopper_frameskip_wl_1[i]
                            frameskip_wl_2 = chopper_frameskip_wl_2[i]
                        else:
                            if frameskip_wl_1<chopper_frameskip_wl_1[i]:
                                frameskip_wl_1=chopper_frameskip_wl_1[i]
                            if frameskip_wl_2>chopper_frameskip_wl_2[i]:
                                frameskip_wl_2=chopper_frameskip_wl_2[i]
            else: 
                frame_srcpulse_wl_1=0.0

        # Get source and detector locations
        source_z = input_ws.getInstrument().getSource().getPos().getZ()
        detector_z = input_ws.getInstrument().getComponentByName("detector1").getPos().getZ()
    
        source_to_detector = (detector_z - source_z)*1000.0
        frame_tof0 = frame_srcpulse_wl_1 / 3.9560346 * source_to_detector ;

        self.log().information("TOF offset = %g microseconds" % frame_tof0)
        self.log().information("Band defined by T1-T4 %g %g" % (frame_wl_1, frame_wl_2))
        self.log().information("Chopper    Actual Phase    Lambda1    Lambda2")
        for i in range(4):
            self.log().information("%d    %g    %g    %g" % (i, chopper_actual_phase[i],
                                                             chopper_wl_1[i], chopper_wl_2[i]))                
        self.setProperty("Offset", frame_tof0)

mtd.registerPyAlgorithm(EQSANSTofOffset())
