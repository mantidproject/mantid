# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This module defines the interface control for EQSANS.
Each reduction method tab that needs to be presented is defined here.
The actual view/layout is define in .ui files. The state of the reduction
process is kept elsewhere (SNSReduction object)
"""

import sys
from mantidqtinterfaces.reduction_gui.instruments.interface import InstrumentInterface
from mantidqtinterfaces.reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from mantidqtinterfaces.reduction_gui.widgets.sans.eqsans_instrument import SANSInstrumentWidget
from mantidqtinterfaces.reduction_gui.widgets.sans.eqsans_data import DataSetsWidget
from mantidqtinterfaces.reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.eqsans_reduction import EQSANSReductionScripter
from mantidqtinterfaces.reduction_gui.widgets.sans.sans_catalog import SANSCatalogWidget

from reduction_gui.reduction.sans.eqsans_catalog import DataCatalog

from reduction_gui.reduction.sans.eqsans_data_proxy import DataProxy

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


class EQSANSInterface(InstrumentInterface):
    """
    Defines the widgets for EQSANS reduction
    """

    data_type = "Data files *.nxs *.dat *.h5 (*.nxs *.dat *.h5)"

    def __init__(self, name, settings):
        super(EQSANSInterface, self).__init__(name, settings)

        self.ERROR_REPORT_NAME = "sans_error_report.xml"
        self.LAST_REDUCTION_NAME = ".mantid_last_reduction.xml"

        # Scripter object to interface with Mantid
        self.scripter = EQSANSReductionScripter(name=name, settings=self._settings)

        # Instrument description
        self.attach(SANSInstrumentWidget(settings=self._settings, data_proxy=DataProxy, data_type=self.data_type))

        # Detector
        self.attach(
            DetectorWidget(
                settings=self._settings,
                data_proxy=DataProxy,
                data_type=self.data_type,
                use_sample_dc=True,
                options_callback=self.scripter.set_options,
            )
        )

        # Sample
        self.attach(DataSetsWidget(settings=self._settings, data_proxy=None, data_type=self.data_type))

        # Catalog
        self.attach(SANSCatalogWidget(settings=self._settings, catalog_cls=DataCatalog))

        # Tabs that only make sense within MantidPlot
        if IS_IN_MANTIDGUI:
            # Stitcher
            self.attach(StitcherWidget(settings=self._settings))

        # Reduction output
        self.attach(OutputWidget(settings=self._settings))

        return

    def has_advanced_version(self):
        """
        Returns true if the instrument has simple and advanced views
        """
        return False
