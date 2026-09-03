# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter

_presenter = None


def show_help_page(relativeUrl, onlineBaseUrl="https://docs.mantidproject.org/"):
    """
    Show the help window at the given relative URL path.
    Local docs path is now determined internally via ConfigService.
    """
    global _presenter
    if _presenter is None:
        _presenter = HelpWindowPresenter(onlineBaseUrl=onlineBaseUrl)

    _presenter.show_help_page(relativeUrl)
