"""
    This module defines the interface control for EQSANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (SNSReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from reduction_gui.widgets.sans.eqsans_instrument import SANSInstrumentWidget
from reduction_gui.widgets.sans.eqsans_data import DataSetsWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.eqsans_reduction import EQSANSReductionScripter
from reduction_gui.widgets.sans.sans_catalog import SANSCatalogWidget

from reduction_gui.reduction.sans.eqsans_catalog import DataCatalog

from reduction_gui.reduction.sans.eqsans_data_proxy import DataProxy
from reduction_gui.widgets.cluster_status import RemoteJobsWidget

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from reduction_gui.widgets.sans.stitcher import StitcherWidget
    IS_IN_MANTIDPLOT = True
except:
    pass

class EQSANSInterface(InstrumentInterface):
    """
        Defines the widgets for EQSANS reduction
    """
    data_type = "Data files *.nxs *.dat (*.nxs *.dat)"

    def __init__(self, name, settings):
        super(EQSANSInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "sans_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = EQSANSReductionScripter(name=name, settings = self._settings)

        # Instrument description
        self.attach(SANSInstrumentWidget(settings = self._settings, data_proxy=DataProxy, data_type = self.data_type))

        # Detector
        self.attach(DetectorWidget(settings = self._settings, data_proxy=None,
                                   data_type = self.data_type, use_sample_dc=True,
                                   options_callback = self.scripter.set_options))

        # Sample
        self.attach(DataSetsWidget(settings = self._settings, data_proxy=None, data_type = self.data_type))

        # Catalog
        self.attach(SANSCatalogWidget(settings = self._settings, catalog_cls=DataCatalog))

        # Tabs that only make sense within MantidPlot
        if IS_IN_MANTIDPLOT:
            # Stitcher
            self.attach(StitcherWidget(settings = self._settings))

        # Reduction output
        self.attach(OutputWidget(settings = self._settings))

        # Tabs that only make sense within MantidPlot
        if IS_IN_MANTIDPLOT:
            # Remote jobs status
            if self.remote_resources_available():
                self.attach(RemoteJobsWidget(settings = self._settings))

    def has_advanced_version(self):
        """
            Returns true if the instrument has simple and advanced views
        """
        return False

    def is_cluster_enabled(self):
        """
            Returns true if the instrument is compatible with remote submission
        """
        return True
