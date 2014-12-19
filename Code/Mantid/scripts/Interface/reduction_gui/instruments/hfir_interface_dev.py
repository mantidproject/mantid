"""
    This module defines the interface control for HFIR SANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (HFIRReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.sans.hfir_instrument import SANSInstrumentWidget
from reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from reduction_gui.widgets.sans.hfir_sample_data import SampleDataWidget
from reduction_gui.widgets.sans.hfir_background import BackgroundWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.hfir_reduction import HFIRReductionScripter
from reduction_gui.widgets.sans.sans_catalog import SANSCatalogWidget

from reduction_gui.reduction.sans.hfir_catalog import DataCatalog

from reduction_gui.reduction.sans.hfir_data_proxy import DataProxy

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from reduction_gui.widgets.sans.stitcher import StitcherWidget
    IS_IN_MANTIDPLOT = True
except:
    pass

class HFIRInterface(InstrumentInterface):
    """
        Defines the widgets for HFIR reduction
    """

    def __init__(self, name, settings):
        super(HFIRInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "sans_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_HFIR_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = HFIRReductionScripter(name=name, settings = self._settings)

        # Instrument description
        self.attach(SANSInstrumentWidget(settings = self._settings, name=name, data_proxy=DataProxy))

        # Detector
        self.attach(DetectorWidget(settings = self._settings, data_proxy=DataProxy,
                                   options_callback = self.scripter.set_options))

        # Sample
        self.attach(SampleDataWidget(settings = self._settings, data_proxy=DataProxy))

        # Background
        self.attach(BackgroundWidget(settings = self._settings, data_proxy=DataProxy))

        # Reduction output
        self.attach(OutputWidget(settings = self._settings))

        # Catalog
        self.attach(SANSCatalogWidget(settings = self._settings, catalog_cls=DataCatalog))

        # Stitcher
        if IS_IN_MANTIDPLOT:
            self.attach(StitcherWidget(settings = self._settings))
