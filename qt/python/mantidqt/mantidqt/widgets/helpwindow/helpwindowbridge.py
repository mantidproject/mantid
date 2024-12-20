import os
from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter
from qtpy.QtCore import QUrl

_presenter = None

online_base_url = "https://docs.mantidproject.org/"
# NOTE: Once you build the html docs using the command ninja docs-html, you will need to
#      use the command export MANTID_LOCAL_DOCS_BASE="/path/to/build/docs/html".
#      If this it not set, this will default to the online docs.
local_docs_base = os.environ.get("MANTID_LOCAL_DOCS_BASE")  # e.g. /path/to/build/docs/html


def show_help_page(relative_url):
    """
    Show the help window at the given relative URL path, e.g. "algorithms/Load-v1.html".
    """
    global _presenter
    if _presenter is None:
        _presenter = HelpWindowPresenter()

    # Default to index.html if no specific file given
    if not relative_url or not relative_url.endswith(".html"):
        relative_url = "index.html"

    if local_docs_base and os.path.isdir(local_docs_base):
        # Use local docs
        full_path = os.path.join(local_docs_base, relative_url)
        file_url = QUrl.fromLocalFile(full_path)
        _presenter.view.browser.setUrl(file_url)
    else:
        # Use online docs
        full_url = online_base_url + relative_url
        _presenter.view.browser.setUrl(QUrl(full_url))

    _presenter.show_help_window()
