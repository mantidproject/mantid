# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import sys


from mantid.kernel import config, logger

if config["default.facility"] != "ILL":
    logger.error("The scan viewer is enabled only if the facility is set to ILL.")
else:
    from mantidqt.gui_helper import get_qapplication
    from mantidqtinterfaces.simplescanviewer.presenter import SimpleScanViewerPresenter

    app, within_mantid = get_qapplication()
    if "SimpleScanViewer" not in globals():
        scan_viewer = SimpleScanViewerPresenter()
    else:
        scan_viewer = globals()["SimpleScanViewer"]
        try:
            visible = scan_viewer.isVisible()
        except RuntimeError:
            # when a scan explorer is closed, the python object can linger while the underlying Qt object has been
            # deleted. In this case, we create a new one from scratch
            visible = False
        if not visible:
            scan_viewer = SimpleScanViewerPresenter()
    scan_viewer.show()
    if not within_mantid:
        sys.exit(app.exec_())
