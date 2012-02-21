from MantidFramework import *
from mantidsimple import *
import math
import os
import numpy

class RefEstimates(PythonAlgorithm):
    
    def category(self):
        return "Reflectometry"

    def name(self):
        return "RefEstimates"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty("RunNumber", 0)
        self.declareProperty("Polarization", True)
        
        self.declareProperty("PeakMin", 0, Direction=Direction.Output)
        self.declareProperty("PeakMax", 0, Direction=Direction.Output)
        self.declareProperty("BackgroundMin", 0, Direction=Direction.Output)
        self.declareProperty("BackgroundMax", 0, Direction=Direction.Output)
        self.declareProperty("MinTOF", 0, Direction=Direction.Output)
        self.declareProperty("MaxTOF", 0, Direction=Direction.Output)

    def PyExec(self):
        run_numbers = self.getProperty("RunNumber")
        if self.getProperty("Polarization"):
            instrument = "REF_M"
        else:
            instrument = "REF_L"
        
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("%s%d"  % (instrument, run_numbers))
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "Could not find run %d\n" % run_numbers
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        self._estimate(data_file, False, True)

        
    def _estimate(self, file_path, is_pixel_y=True, high_res = True):
        basename = os.path.basename(file_path)
        ws_base = "__%s" % basename
        ws_output = "%s_integrated" % ws_base
            
        def _load_entry(entry, is_first=True):
            if is_first:
                ws = ws_output
            else:
                ws = "%s_%s" % (ws_output, entry)
            
            LoadEventNexus(Filename=file_path, OutputWorkspace=ws,
                           NXentryName=entry)
            if mtd[ws].getNumberEvents()==0:
                mtd.deleteWorkspace(ws)
                return False

            if is_first:
                # Get TOF range before grouping detectors
                ws_summed = ws+'_summed'
                Rebin(InputWorkspace=ws, OutputWorkspace=ws_summed,Params="0,500,200000")
                SumSpectra(InputWorkspace=ws_summed, OutputWorkspace=ws_summed)
                x = mtd[ws_summed].readX(0)
                y = mtd[ws_summed].readY(0)
                sum = 0
                wsum = 0
                dsum = 0
                for i in range(len(y)):
                    sum += y[i]
                    wsum += x[i]*y[i]
                    
                mean = wsum/sum
                for i in range(len(y)):
                    dsum += y[i]*(x[i]-mean)*(x[i]-mean)
                sigma = math.sqrt(dsum/sum)
                
                #mean, sigma = self._find_peak(ws_summed, True)
                min = mean-1.5*sigma
                max = mean+1.5*sigma          
                self.setProperty("MinTOF", int(math.ceil(min)))
                self.setProperty("MaxTOF", int(math.ceil(max)))

            # 1D plot
            instr_dir = mtd.getSettings().getInstrumentDirectory()
            if is_pixel_y:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             "REFL_Detector_Grouping_Sum_X.xml")
                GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws,
                               MapFile=grouping_file)
            else:
                grouping_file = os.path.join(instr_dir, "Grouping",
                                             "REFL_Detector_Grouping_Sum_Y.xml")
                GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws,
                               MapFile=grouping_file)
                
            Transpose(InputWorkspace=ws, OutputWorkspace=ws)
            
            # The Y pixel numbers start at 1 from the perspective of the users
            # They also read in reversed order
            if is_pixel_y:
                x=mtd[ws].dataX(0)
                y_reversed=mtd[ws].dataY(0)
                y=[i for i in y_reversed]
                for i in range(len(x)):
                    x[i] += 1
                    y_reversed[i] = y[len(y)-1-i]
                
            if not is_first:
                Plus(LHSWorkspace=ws_output, RHSWorkspace=ws,
                     OutputWorkspace=ws_output)
            
            return True
            
        pols = ['']
        if self.getProperty("Polarization"):
            pols = ['entry-Off_Off', 'entry-Off_On',
                    'entry-On_Off', 'entry-On_On']

        is_first = True
        for p in pols:
            if _load_entry(p, is_first):
                is_first = False      
                
        mean, sigma = self._find_peak(ws_output, high_res)
        min = mean-2*sigma
        max = mean+2*sigma          
        self.setProperty("PeakMin", int(math.ceil(min)))
        self.setProperty("PeakMax", int(math.ceil(max)))
        
        min = mean-6*sigma
        max = mean+6*sigma
        self.setProperty("BackgroundMin", int(math.ceil(min)))
        self.setProperty("BackgroundMax", int(math.ceil(max)))
           
    def _find_peak(self, ws_output, high_res=True):
        # Estimate peak limits            
        x = mtd[ws_output].readX(0)
        if not high_res:
            Rebin(InputWorkspace=ws_output, OutputWorkspace='__'+ws_output,
                  Params="0,10,%d" % len(x))
            ws_output = '__'+ws_output
            x = mtd[ws_output].readX(0)
        y = mtd[ws_output].readY(0)
        
        n=[]
        slope_data=[]
        sum = 0.0
        min_id = 0
        max_id = 0
        max_slope = 0
        min_slope = 0
        for i in range(len(y)):
            sum += y[i]
            n.append(sum)
            if i>0:
                slope_data.append(y[i]-y[i-1])
                if slope_data[i-1]<min_slope:
                    min_slope=slope_data[i-1]
                    min_id = x[i-1]
                if slope_data[i-1]>max_slope:
                    max_slope = slope_data[i-1]
                    max_id = x[i-1]
                    
        sigma = (min_id-max_id)/2.0
        mean = (min_id+max_id)/2.0

        return mean, sigma

    
mtd.registerPyAlgorithm(RefEstimates())
