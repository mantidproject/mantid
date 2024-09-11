# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This module defines the interface control for HFIR SANS.
Each reduction method tab that needs to be presented is defined here.
The actual view/layout is define in .ui files. The state of the reduction
process is kept elsewhere (HFIRReduction object)
"""

import sys
from mantidqtinterfaces.reduction_gui.instruments.interface import InstrumentInterface
from mantidqtinterfaces.reduction_gui.widgets.sans.hfir_instrument import SANSInstrumentWidget
from mantidqtinterfaces.reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from mantidqtinterfaces.reduction_gui.widgets.sans.hfir_sample_data import SampleDataWidget
from mantidqtinterfaces.reduction_gui.widgets.sans.hfir_background import BackgroundWidget
from mantidqtinterfaces.reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.hfir_reduction import HFIRReductionScripter
from mantidqtinterfaces.reduction_gui.widgets.sans.sans_catalog import SANSCatalogWidget

from reduction_gui.reduction.sans.hfir_catalog import DataCatalog

from reduction_gui.reduction.sans.hfir_data_proxy import DataProxy

IS_IN_MANTIDGUI = False
if "workbench.app.mainwindow" in sys.modules:
    IS_IN_MANTIDGUI = True
else:
    try:
        import mantidplot  # noqa

        IS_IN_MANTIDGUI = True
    except:
        pass
if IS_IN_MANTIDGUI:
    from mantidqtinterfaces.reduction_gui.widgets.sans.stitcher import StitcherWidget


class HFIRInterface(InstrumentInterface):
    """
    Defines the widgets for HFIR reduction
    """

    def __init__(self, name, settings):
        super(HFIRInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "sans_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_HFIR_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = HFIRReductionScripter(name=name, settings=self._settings)

        # Instrument description
        self.attach(SANSInstrumentWidget(settings=self._settings, name=name, data_proxy=DataProxy))

        # Detector
        self.attach(DetectorWidget(settings=self._settings, data_proxy=DataProxy, options_callback=self.scripter.set_options))

        # Sample
        self.attach(SampleDataWidget(settings=self._settings, data_proxy=DataProxy))

        # Background
        self.attach(BackgroundWidget(settings=self._settings, data_proxy=DataProxy))

        # Reduction output
        self.attach(OutputWidget(settings=self._settings))

        # Catalog
        self.attach(SANSCatalogWidget(settings=self._settings, catalog_cls=DataCatalog))

        # Stitcher
        if IS_IN_MANTIDGUI:
            self.attach(StitcherWidget(settings=self._settings))
