from interface import InstrumentInterface
from reduction_gui.widgets.inelastic.dgs_sample_setup import SampleSetupWidget
from reduction_gui.widgets.inelastic.dgs_data_corrections import DataCorrectionsWidget
from reduction_gui.widgets.inelastic.dgs_diagnose_detectors import DiagnoseDetectorsWidget
from reduction_gui.widgets.inelastic.dgs_absolute_units import AbsoluteUnitsWidget
from reduction_gui.widgets.inelastic.dgs_pd_sc_conversion import PdAndScConversionWidget
from reduction_gui.widgets.cluster_status import RemoteJobsWidget
from reduction_gui.reduction.inelastic.dgs_reduction_script import DgsReductionScripter
class DgsInterface(InstrumentInterface):
    """
        Defines the widgets for direct geometry spectrometer reduction
    """
    # Allowed extensions for loading data files
    data_type = "Data files *.* (*.*)"


    def __init__(self, name, settings):
        super(DgsInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "dgs_error_report.xml"

        # Scripter object to interface with Mantid
        self.scripter = DgsReductionScripter(name=name, facility=settings.facility_name)

        # Sample run setup
        self.attach(SampleSetupWidget(settings = self._settings,
                                      data_type = self.data_type))

        # Data corrections
        self.attach(DataCorrectionsWidget(settings = self._settings,
                                          data_type = self.data_type))

        # Diagnose detectors
        self.attach(DiagnoseDetectorsWidget(settings = self._settings,
                                            data_type = self.data_type))

        # Absolute units normalisation
        self.attach(AbsoluteUnitsWidget(settings = self._settings,
                                        data_type = self.data_type))

        # Powder and Single Crystal conversion
        #self.attach(PdAndScConversionWidget(settings = self._settings,
        #                                    data_type = self.data_type))

        # Remote jobs status
        if self.remote_resources_available():
            self.attach(RemoteJobsWidget(settings = self._settings))

    def is_cluster_enabled(self):
        """
            Returns true if the instrument is compatible with remote submission
        """
        return True
