from MantidFramework import *
from mantidsimple import *
from DirectEnergyConversion import setup_reducer

class dgreduce(PythonAlgorithm):
    def category(self):
        return "Inelastic"

    def name(self):
        return "dgreduce"

    def PyInit(self):
        instruments = ["", "ARCS", "CNCS", "LET", "MAPS", "MARI", "MERLIN", "SEQUOIA"]
        self.declareProperty("Instrument", "",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareListProperty("Run", [0], Validator=BoundedValidator(Lower=0))
        self.declareProperty("Ei",0.0,Description="Incident Energy Guess")
        self.declareProperty("EnergyBins","",Description="Energy bins [Emin,Estep,Emax] for final output (meV). Positive is linear bins, negative is logorithmic")
        
        self.declareProperty("DetectorVanadium", "", Description="Vanadium to use for Detector Normalisation")
        self.declareProperty("FixEi", False, Description="Fix")
#        self.declareFileProperty("MapFile", "", FileAction.Load, ['.map'],
#                                 Description="Mapping file for sample.")
#        self.declareFileProperty("AbsMapFile", "", FileAction.Load, ['.map'],
#                                 Description="Mapping file for absolute vanadium.")
        self.declareProperty("MapFile", "", 
                                 Description="Mapping file for sample.")
        self.declareProperty("AbsMapFile", "", 
                                 Description="Mapping file for absolute vanadium.")
        self.declareProperty("AbsDetectorVanadium", "", Description="Vanadium to use for Detector Normalisation for Absolute Units.")

        
        self.declareProperty("RunDiag", True, Description="Run the Diagnostics to produce a mask.")
        
        # Output Formats
        self.declareProperty("NXSPE", False, Description="Save output as NXSPE file.")
        self.declareProperty("SPE", False, Description="Save output as SPE file.")
        self.declareProperty("MantidNeXus", False, Description="Save output as a Mantid NeXus file.")
        
        
    def PyExec(self):
        self._instrument = self.getProperty("Instrument")
        
        self._run = self.getProperty("Run")
        
        self._ei = self.getProperty("Ei")
        self._energy_bins = self.getProperty("EnergyBins")
        self._detector_vanadium = self.getProperty("DetectorVanadium")    
        if (self._detector_vanadium == ""):
            self._detector_vanadium = None
                
        self._abs_detector_vanadium = self.getProperty("AbsDetectorVanadium")
        self._diag = self.getProperty("RunDiag")
        
        reducer = setup_reducer(self._instrument)
        
        reducer.save_formats = []

        if (self._diag):
            reducer.spectra_masks = reducer.diagnose(self._detector_vanadium, self._run, remove_zero=True, tiny=0.0,
                                                     large=1e10, median_lo=0.1, median_hi=3.0, signif=3.3, 
                                                     bkgd_threshold=5.0, variation=1.1)
        
        resultws = reducer.convert_to_energy(mono_run=self._run, ei=self._ei, white_run=self._detector_vanadium, 
                                             abs_white_run=self._abs_detector_vanadium)
            
    
mtd.registerPyAlgorithm(dgreduce())