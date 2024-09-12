# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
"""
Test interface for RegionSelector. This will be removed once the
widget is embedded into ISISReflectometry - Preview tab.
"""

from typing import List
from mantid.api import AnalysisDataService

from mantidqt.widgets.regionselector.presenter import RegionSelector


ads = AnalysisDataService.Instance()
workspace_names: List[str] = ads.getObjectNames()
if len(workspace_names) < 1:
    workspace = None
else:
    # Just get any workspace from the ADS to test with
    workspace = ads.retrieve(workspace_names[0])
presenter = RegionSelector(ws=workspace)
presenter.view.show()
