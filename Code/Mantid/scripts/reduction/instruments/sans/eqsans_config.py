"""
    Parser for EQSANS configuration file
"""
import os
import re

class EQSANSConfig(object):
    
    def __init__(self, file_path=None):
        
        if file_path is not None and not os.path.isfile(file_path):
            raise RuntimeError, "Provided path is not valid: %s" % str(file_path)
        
        ## File path
        if file_path is not None and not os.path.isfile(file_path):
            raise RuntimeError, "Provided path is not valid: %s" % str(file_path)
        self._file_path = file_path
        
        self.reset()
        if file_path is not None:
            self._process_file()
    
    def reset(self):
        """
            Reset the data members
        """
        ## Rectangular masks
        self.rectangular_masks = []
        
        ## TOF band to discard at the beginning of the frame
        self.low_TOF_cut = 0
        
        ## TOF band to discard at the end of the frame
        self.high_TOF_cut = 0
        
        # Beam center
        self.center_x = None
        self.center_y = None
        
        # Sample-detector distance
        self.sample_detector_dist = 0
        
        # Prompt pulse width
        self.prompt_pulse_width = None
        
        # Slit positions
        self.slit_positions = [8*[20.0],8*[20.0],8*[20.0]]
        
        # Moderator position
        self.moderator_position = None
    
    def _process_file(self):
        """
            Read and process the configuration file
        """
        self.reset()
        f = open(self._file_path)
        for line in f.readlines():
            
            # Looking for rectangular mask
            # Rectangular mask         = 7, 0; 7, 255
            #FIXME: Elliptical masks are treat as rectangular masks until implemented
            if not line.strip().startswith("#") and  (line.lower().find("rectangular mask")>=0 \
                or line.find("Elliptical mask")>=0):
                coord = re.search("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)[ ]*[ ;,][ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)", line)
                if coord is not None:
                    try:
                        x1 = int(coord.group(1))
                        y1 = int(coord.group(2))
                        x2 = int(coord.group(3))
                        y2 = int(coord.group(4))
                        self.rectangular_masks.append([x1,x2,y1,y2])
                    except:
                        # Badly formed line, skip it
                        pass
                    
            # Looking for TOF band to cut from each side of the frame
            if not line.strip().startswith("#") and line.lower().find("tof edge discard")>=0:
                cut = re.search("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)", line)
                if cut is not None:
                    self.low_TOF_cut = float(cut.group(1))
                    self.high_TOF_cut = float(cut.group(2))

            # Looking for beam center
            #Spectrum center            = 89.6749, 129.693 [pixel]
            if line.lower().find("spectrum center")>=0:
                ctr = re.search("=[ ]*([0-9]+.[0-9]*)[ ]*[ ,][ ]*([0-9]+.[0-9]+)", line)
                if ctr is not None:
                    self.center_x = float(ctr.group(1))
                    self.center_y = float(ctr.group(2))

            # Sample-detector distance
            #Sample to detector        = 4000 [mm]
            if line.lower().find("sample to detector")>=0:
                dist = re.search("=[ ]*([0-9]+.?[0-9]*)", line)
                if dist is not None:
                    self.sample_detector_dist = float(dist.group(1))

            # Prompt pulse width
            #Prompt pulse halfwidth          = 20 [microseconds]
            if line.lower().find("prompt pulse")>=0:
                width = re.search("=[ ]*([0-9]+.?[0-9]*)", line)
                if width is not None:
                    self.prompt_pulse_width = float(width.group(1))
                    
            # Slits
            if line.lower().find("wheel")>=0:
                slit = re.search("([1-8]) wheel[ ]*([1-3])[ \t]*=[ \t]*(\w+)", line.lower())
                if slit is not None:
                    slit_number = int(slit.group(1))-1
                    wheel_number = int(slit.group(2))-1
                    slit_name = slit.group(3)
                    slit_size = 0.0
                    slit_size_re = re.search("\w*?([0-9]+)mm", slit_name)
                    if slit_size_re is not None:
                        slit_size = float(slit_size_re.group(1))
                    self.slit_positions[wheel_number][slit_number] = slit_size      
                    
            # Distance between moderator and sample
            # Sample Location         = 14160
            if line.lower().find("sample location")>=0:
                pos = re.search("=[ ]*([0-9]+)", line)
                if pos is not None:
                    self.moderator_position = float(pos.group(1))
