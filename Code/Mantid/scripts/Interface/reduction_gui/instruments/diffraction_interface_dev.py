from interface import InstrumentInterface

from reduction_gui.widgets.diffraction.diffraction_run_setup import RunSetupWidget
from reduction_gui.widgets.diffraction.diffraction_van_setup import VanadiumSetupWidget
from reduction_gui.widgets.diffraction.diffraction_filter_setup import FilterSetupWidget

# from reduction_gui.widgets.inelastic.dgs_data_corrections import DataCorrectionsWidget
# from reduction_gui.widgets.inelastic.dgs_diagnose_detectors import DiagnoseDetectorsWidget
# from reduction_gui.widgets.inelastic.dgs_absolute_units import AbsoluteUnitsWidget
# from reduction_gui.widgets.inelastic.dgs_pd_sc_conversion import PdAndScConversionWidget

from reduction_gui.reduction.diffraction.diffraction_reduction_script import DiffractionReductionScripter

class DiffractionInterface(InstrumentInterface):
    """
        Defines the widgets for direct geometry spectrometer reduction
    """
    # Allowed extensions for loading data files
    data_type = "Data files *.* (*.*)"

    
    def __init__(self, name, settings):
        super(DiffractionInterface, self).__init__(name, settings)
        
        self.ERROR_REPORT_NAME = "diffraction_error_report.xml"
        
        # Scripter object to interface with Mantid 
        self.scripter = DiffractionReductionScripter(name=name, facility=settings.facility_name)        

        # Tab Test: Sample run setup (Test case.  Will be removed later)
        # self.attach(SampleSetupWidget(settings = self._settings, data_type = self.data_type))

        # Tab 1: Run number setup (Will be the first one)
        self.attach(RunSetupWidget(settings = self._settings, data_type = self.data_type))

        # Tab 2: Advanced and Vanadium number setup 
        self.attach(VanadiumSetupWidget(settings = self._settings, data_type = self.data_type))
        
        # Tab 3: Event filters setup
        self.attach(FilterSetupWidget(settings = self._settings, data_type = self.data_type))
        
        # Data corrections
        # self.attach(DataCorrectionsWidget(settings = self._settings,
        #                                   data_type = self.data_type))
        
        # Diagnose detectors
        # self.attach(DiagnoseDetectorsWidget(settings = self._settings, 
        #                                     data_type = self.data_type))
        
        # Absolute units normalisation
        # self.attach(AbsoluteUnitsWidget(settings = self._settings,
        #                                 data_type = self.data_type))
        
        # Powder and Single Crystal conversion
        #self.attach(PdAndScConversionWidget(settings = self._settings,
        #                                    data_type = self.data_type))

        return
