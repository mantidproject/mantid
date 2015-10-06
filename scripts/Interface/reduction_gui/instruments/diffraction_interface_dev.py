from interface import InstrumentInterface

from reduction_gui.widgets.diffraction.diffraction_run_setup import RunSetupWidget
from reduction_gui.widgets.diffraction.diffraction_adv_setup import AdvancedSetupWidget
from reduction_gui.widgets.diffraction.diffraction_filter_setup import FilterSetupWidget
from reduction_gui.widgets.cluster_status import RemoteJobsWidget

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

        # Tab 1: Run number setup (Will be the first one)
        self.attach(RunSetupWidget(settings = self._settings, data_type = self.data_type))

        # Tab 2: Advanced and Vanadium number setup
        self.attach(AdvancedSetupWidget(settings = self._settings, data_type = self.data_type))

        # Tab 3: Event filters setup
        self.attach(FilterSetupWidget(settings = self._settings, data_type = self.data_type))

        # Remote jobs status
        if self.remote_resources_available():
            self.attach(RemoteJobsWidget(settings = self._settings))
        return

    def is_cluster_enabled(self):
        """
            Returns true if the instrument is compatible with remote submission
        """
        return True
