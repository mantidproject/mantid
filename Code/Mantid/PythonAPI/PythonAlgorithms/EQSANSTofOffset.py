from MantidFramework import *
from mantidsimple import *

class EQSANSTofOffset(PythonAlgorithm):
    
    # Source repetition rate (Hz)
    REP_RATE = 60.0
    PULSEWIDTH = 20.0    #micro sec per angstrom
    #CHOPPER_PHASE_OFFSET = [[9507.,9471.,9829.7,9584.3],[19024.,18820.,19714.,19360.]]    # micro sec
    CHOPPER_PHASE_OFFSET = [[9507.,-23862.,-23503.5,-23749.2],[19024.,18820.,19714.,19360.]]    # micro sec
    CHOPPER_ANGLE = [129.605,179.989,230.010,230.007] # degree
    CHOPPER_LOCATION = [5700.,7800.,9497.,9507.]     # mm
    
    def category(self):
        return "SANS"

    def name(self):
        return "EQSANSTofOffset"

    def PyInit(self):
        self.declareProperty("InputWorkspace", "")
        self.declareProperty("FrameSkipping", False)
        self.declareProperty("Offset", 0.0, Direction=Direction.Output)

    def PyExec(self):

        input_ws = self.getProperty("InputWorkspace")
        frame_skipping = self.getProperty("FrameSkipping")

        # Storage for chopper informate read from the logs
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
            chopper_set_phase[i] = mtd[input_ws].getRun()[log_str].getStatistics().mean
            log_str = 'Speed'+str(i+1)
            chopper_speed[i] = mtd[input_ws].getRun()[log_str].getStatistics().mean
            
            chopper_actual_phase[i]=chopper_set_phase[i];
            if not ( chopper_speed[i]==0. and chopper_set_phase[i]==0. ):
                chopper_actual_phase[i] -= self.CHOPPER_PHASE_OFFSET[m_set][i]
            
            while chopper_actual_phase[i]<0:
                chopper_actual_phase[i] += tmp_frame_width
    
            if chopper_speed[i]>0:
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
    
        source_to_detector = (14.0+8.0)*1000.0
        frame_tof0 = frame_srcpulse_wl_1 / 3.9560346 * source_to_detector ;

        self.log().information("TOF offset = %g microseconds" % frame_tof0)
        self.log().information("Band defined by T1-T4 %g %g" % (frame_wl_1, frame_wl_2))
        self.log().information("Chopper    Actual Phase    Lambda1    Lambda2")
        for i in range(4):
            self.log().information("%d    %g    %g    %g" % (i, chopper_actual_phase[i],
                                                             chopper_wl_1[i], chopper_wl_2[i]))                
        self.setProperty("Offset", frame_tof0)

mtd.registerPyAlgorithm(EQSANSTofOffset())
