from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter
from qtpy.QtCore import QUrl

_presenter = None


def show_help_page(relative_url):
    """
    Show the help window at the given relative URL path, e.g. "algorithms/Load-v1.html".
    """
    global _presenter
    if _presenter is None:
        _presenter = HelpWindowPresenter()
    full_url = "https://docs.mantidproject.org/" + relative_url
    _presenter.view.browser.setUrl(QUrl(full_url))
    _presenter.show_help_window()
