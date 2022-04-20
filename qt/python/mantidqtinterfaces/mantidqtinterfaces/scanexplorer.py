# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import sys


from mantid.kernel import config, logger

if config['default.facility'] != 'ILL':
    logger.error('ScanExplorer is enabled only if the facility is set to ILL.')
else:
    from mantidqt.gui_helper import get_qapplication
    from mantidqtinterfaces.scanexplorer.presenter import ScanExplorerPresenter

    app, within_mantid = get_qapplication()
    if 'scanExplorer' not in globals():
        scan_explorer = ScanExplorerPresenter()
    else:
        scan_explorer = globals()["scanExplorer"]
        try:
            visible = scan_explorer.isVisible()
        except:
            # underlying Qt object has been deleted
            visible = False
        if not visible:
            scan_explorer = ScanExplorerPresenter()
    scan_explorer.show()
    if not within_mantid:
        sys.exit(app.exec_())
