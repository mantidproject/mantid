from MantidFramework import *
from mantidsimple import *
import os
import numpy
from reduction.instruments.reflectometer import wks_utility

class RefLReduction(PythonAlgorithm):

    def category(self):
        return "Diffraction;PythonAlgorithms"

    def name(self):
        return "RefLReduction"

    def PyInit(self):
        self.declareListProperty("RunNumbers", [0], Validator=ArrayBoundedValidator(Lower=0))
        
        self.declareProperty("NormalizationRunNumber", 0, Description="")

        self.declareListProperty("SignalPeakPixelRange", [126, 134], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("SignalBackgroundPixelRange", [123, 137], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("NormPeakPixelRange", [127, 133], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("NormBackgroundPixelRange", [123, 137], Validator=ArrayBoundedValidator(Lower=0))
        self.declareListProperty("LowResAxisPixelRange", [115, 210], Validator=ArrayBoundedValidator(Lower=0))

        self.declareListProperty("TOFRange", [9000., 23600.], Validator=ArrayBoundedValidator(Lower=0))
        

        self.declareListProperty("Binning", [0,200,200000],
                             Description="Positive is linear bins, negative is logorithmic")

    def _findData(self, runnumber, extension):
        result = FindSNSNeXus(Instrument=self._instrument,
                              RunNumber=runnumber, Extension=extension)
        return result["ResultPath"].value


    def _loadNeXusData(self, filename, name, bank, extension, **kwargs):
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, BankName=bank, SingleBankPixelsOnly=1, FilterByTofMin=self._binning[0], FilterByTofMax=self._binning[2], LoadMonitors=True, MonitorsAsEvents=True, **kwargs)
        wksp = alg['OutputWorkspace']
        LoadIsawDetCal(InputWorkspace=wksp,Filename=self._DetCalfile)
        #Normalise by sum of counts in upstream monitor
        Integration(InputWorkspace=mtd[str(name)+'_monitors'], OutputWorkspace='Mon', RangeLower=self._binning[0], RangeUpper=self._binning[2], EndWorkspaceIndex=0)
        mtd.deleteWorkspace(str(name)+'_monitors')
        mtd.releaseFreeMemory()
        # take care of filtering events
        if self._filterBadPulses:
            FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
        CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns
        return wksp

    def _findNeXusData(self, runnumber, bank, extension, **kwargs):
        kwargs["Precount"] = True
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension
        return self._loadNeXusData(filename, name, bank, extension, **kwargs)

    def _loadData(self, runnumber, bank, extension, filterWall=None):
        filter = {}
        if filterWall is not None:
            if filterWall[0] > 0.:
                filter["FilterByTofStart"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTofStop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._findNeXusData(runnumber, bank, extension, **filter)
        else:
            return self._findPreNeXusData(runnumber, extension)

  

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        self._binning = self.getProperty("Binning")
        if len(self._binning) != 1 and len(self._binning) != 3:
            raise RuntimeError("Can only specify (width) or (start,width,stop) for binning. Found %d values." % len(self._binning))
        if len(self._binning) == 3:
            if self._binning[0] == 0. and self._binning[1] == 0. and self._binning[2] == 0.:
                raise RuntimeError("Failed to specify the binning")
        
        run_numbers = self.getProperty("RunNumbers")
        allow_multiple = False
        if len(run_numbers)>1 and not allow_multiple:
            raise RuntimeError("Not ready for multiple runs yet, please specify only one run number")
    
        normalization_run = self.getProperty("NormalizationRunNumber")
    
        data_peak = self.getProperty("SignalPeakPixelRange")
        data_back = self.getProperty("SignalBackgroundPixelRange")

        TOFrange = self.getProperty("TOFRange") #microS
        
        #Due to the frame effect, it's sometimes necessary to narrow the range
        #over which we add all the pixels along the low resolution
        #Parameter
        Xrange = self.getProperty("LowResAxisPixelRange")
        
        h = 6.626e-34  #m^2 kg s^-1
        m = 1.675e-27     #kg
        
        #dimension of the detector (256 by 304 pixels)
        maxX = 304
        maxY = 256
        
        norm_back = self.getProperty("NormBackgroundPixelRange")
        BackfromYpixel = norm_back[0]
        BacktoYpixel = norm_back[1]

        norm_peak = self.getProperty("NormPeakPixelRange")
        from_peak = norm_peak[0]
        to_peak = norm_peak[1]
        
        ########################################################################
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_L%d" %run_numbers[0])
        if len(f)>0 and os.path.isfile(f[0]): 
            data_file = f[0]
        else:
            msg = "RefLReduction: could not find run %d\n" % run_number[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
        
        # Pick a good workspace name
        ws_name = "refl%d" % run_numbers[0]
        ws_event_data = ws_name+"_evt"  
        # Load the data into its workspace
        LoadEventNexus(Filename=data_file, OutputWorkspace=ws_event_data)
        
        # Get metadata
        mt_run = mtd[ws_event_data].getRun()
        ##get angles value
        ths_value = mt_run.getProperty('ths').value[0]
        ths_units = mt_run.getProperty('ths').units
        tthd_value = mt_run.getProperty('tthd').value[0]
        tthd_units = mt_run.getProperty('tthd').units
        ths_rad = wks_utility.angleUnitConversion(value=ths_value,
                                                  from_units=ths_units,
                                                  to_units='rad')
        tthd_rad = wks_utility.angleUnitConversion(value=tthd_value,
                                                   from_units=tthd_units,
                                                   to_units='rad')
        # Retrieve geometry of instrument
        # Sample-to-detector distance
        sample = mtd[ws_event_data].getInstrument().getSample()
        source = mtd[ws_event_data].getInstrument().getSource()
        dSM = sample.getDistance(source)
        # Create array of distances pixel->sample
        dPS_array = numpy.zeros((maxY, maxX))
        for x in range(maxX):
            for y in range(maxY):
                _index = maxY * x + y
                detector = mtd[ws_event_data].getDetector(_index)
                dPS_array[y, x] = sample.getDistance(detector)
        # Array of distances pixel->source
        dMP_array = dPS_array + dSM
        # Distance sample->center of detector
        dSD = dPS_array[maxY / 2, maxX / 2]
        # Distance source->center of detector
        dMD = dSD + dSM
        
        
        # Rebin data (x-axis is in TOF)
        ws_histo_data = ws_name+"_histo"
        Rebin(InputWorkspace=ws_event_data, OutputWorkspace=ws_histo_data, Params=self._binning)
        
        # Keep only range of TOF of interest
        CropWorkspace(ws_histo_data,ws_histo_data,XMin=TOFrange[0], XMax=TOFrange[1])
        
        # Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace=ws_histo_data, OutputWorkspace=ws_histo_data)
        
        # Calculation of the central pixel (using weighted average)
        pixelXtof_data = wks_utility.getPixelXTOF(mtd[ws_histo_data], maxX=maxX, maxY=maxY)
        pixelXtof_1d = pixelXtof_data.sum(axis=1)
        # Keep only range of pixels
        pixelXtof_roi = pixelXtof_1d[data_peak[0]:data_peak[1]]
        sz = pixelXtof_roi.size
        _num = 0
        _den = 0
        start_pixel = data_peak[0]
        for i in range(sz):
            _num += (start_pixel * pixelXtof_roi[i])
            start_pixel = start_pixel + 1
            _den += pixelXtof_roi[i]
        data_cpix = _num / _den    
        
        # Background subtraction
        BackfromYpixel = data_back[0]
        BacktoYpixel = data_back[1]
        
        #Create a new event workspace of only the range of pixel of interest 
        #background range (along the y-axis) and of only the pixel
        #of interest along the x-axis (to avoid the frame effect)
        mt2 = wks_utility.createIntegratedWorkspace(mtd[ws_histo_data], 
                                                    "IntegratedDataWks",
                                                    fromXpixel=Xrange[0],
                                                    toXpixel=Xrange[1],
                                                    fromYpixel=BackfromYpixel,
                                                    toYpixel=BacktoYpixel,
                                                    maxX=maxX,
                                                    maxY=maxY)
        
        theta = tthd_rad - ths_rad
        _tof_axis = mt2.readX(0)[:]
        ########## This was used to test the R(Q) 
        ##Convert the data without background subtraction to R(Q)
        q_array = wks_utility.convertToRvsQ(dMD=dMD,
                                            theta=theta,
                                            tof=_tof_axis)
        
        q_array_reversed = q_array[::-1]
        
        # Background
        Transpose(InputWorkspace='IntegratedDataWks',
                  OutputWorkspace='TransposedID')
        ConvertToHistogram(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID')
        FlatBackground(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedFlatID',
                       StartX=BackfromYpixel,
                       Mode='Mean',
                       EndX=data_peak[0])
        Transpose(InputWorkspace='TransposedFlatID',
                  OutputWorkspace='DataWks')
            
            
        # Work on Normalization file
        # Find full path to event NeXus data file
        f = FileFinder.findRuns("REF_L%d" %normalization_run)
        if len(f)>0 and os.path.isfile(f[0]): 
            norm_file = f[0]
        else:
            msg = "RefLReduction: could not find run %d\n" % run_number[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)
            
        #load normalization file
        ws_norm = "__normalization_refl%d" % run_numbers[0]
        ws_norm_event_data = ws_norm+"_evt"  

        LoadEventNexus(Filename=norm_file, OutputWorkspace=ws_norm_event_data)
    
        # Rebin data
        Rebin(InputWorkspace=ws_norm_event_data,
              OutputWorkspace='HistoNorm',
              Params=self._binning)
    
        ##keep only range of TOF of interest
        CropWorkspace('HistoNorm', 
                      'CropHistoNorm', 
                      XMin=TOFrange[0], 
                      XMax=TOFrange[1])
    
        #Normalized by Current (proton charge)
        NormaliseByCurrent(InputWorkspace='CropHistoNorm', 
                           OutputWorkspace='NormWks')
        mt = mtd['NormWks']
    
        ##Background subtraction

        #Create a new event workspace of only the range of pixel of interest 
        #background range (along the y-axis) and of only the pixel
        #of interest along the x-axis (to avoid the frame effect)
        mt3_norm = wks_utility.createIntegratedWorkspace(mt, "IntegratedNormWks",
                                            fromXpixel=Xrange[0],
                                            toXpixel=Xrange[1],
                                            fromYpixel=BackfromYpixel,
                                            toYpixel=BacktoYpixel,
                                            maxX=maxX,
                                            maxY=maxY)

        Transpose(InputWorkspace='IntegratedNormWks',
                  OutputWorkspace='TransposedID')
        
        ConvertToHistogram(InputWorkspace='TransposedID',
                           OutputWorkspace='TransposedID')
        
        FlatBackground(InputWorkspace='TransposedID',
                       OutputWorkspace='TransposedFlatID',
                       StartX=BackfromYpixel,
                       Mode='Mean',
                       EndX=norm_peak[0])

        Transpose(InputWorkspace='TransposedFlatID',
                  OutputWorkspace='NormWks')
   
        
    
        #perform the integration myself
        mt_temp = mtd['NormWks']
        x_axis = mt_temp.readX(0)[:]   #[9100,9300,.... 23500] (73,1)
        NormPeakRange = numpy.arange(to_peak-from_peak+1) + from_peak
        counts_vs_tof = numpy.zeros(len(x_axis))

        # Normalization           
        SumSpectra(InputWorkspace="NormWks", OutputWorkspace="NormWks")
        #### divide data by normalize histo workspace
        Divide(LHSWorkspace='DataWks',
               RHSWorkspace='NormWks',
               OutputWorkspace='NormalizedWks')
        ReplaceSpecialValues("NormalizedWks",NaNValue=0,NaNError=0, OutputWorkspace="NormalizedWks")
        SumSpectra(InputWorkspace="NormalizedWks", OutputWorkspace="Reflectivity")
            
mtd.registerPyAlgorithm(RefLReduction())
