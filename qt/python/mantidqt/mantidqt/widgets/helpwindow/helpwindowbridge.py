# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter
from qtpy.QtCore import QUrl
from qtpy.QtWidgets import QApplication


_presenter = None


def show_help_page(relative_url, local_docs=None, online_base_url="https://docs.mantidproject.org/"):
    """
    Show the help window at the given relative URL path
    """
    global _presenter
    if _presenter is None:
        _presenter = HelpWindowPresenter()

    if not relative_url or not relative_url.endswith(".html"):
        relative_url = "index.html"

    if local_docs and os.path.isdir(local_docs):
        # Use local docs
        full_path = os.path.join(local_docs, relative_url)
        file_url = QUrl.fromLocalFile(full_path)
        _presenter.view.browser.setUrl(file_url)
    else:
        # Use online docs
        full_url = online_base_url.rstrip("/") + "/" + relative_url
        _presenter.view.browser.setUrl(QUrl(full_url))

    _presenter.show_help_window()


def main(cmdargs=None):
    """
    Run this script standalone to test the Python-based Help Window.
    """
    import argparse

    parser = argparse.ArgumentParser(description="Standalone test of the Python-based Mantid Help Window.")
    parser.add_argument(
        "relative_url", nargs="?", default="", help="Relative doc path (like 'algorithms/Load-v1.html'), defaults to 'index.html' if empty."
    )
    parser.add_argument("--local-docs", default=None, help="Path to local Mantid HTML docs. If valid, the viewer will load docs from here.")
    parser.add_argument(
        "--online-base-url", default="https://docs.mantidproject.org/", help="Online docs base URL if local docs are not set or invalid."
    )
    args = parser.parse_args(cmdargs or sys.argv[1:])

    if args.local_docs is None:
        # e.g. MANTID_LOCAL_DOCS_BASE is /path/to/build/docs/html
        args.local_docs = os.environ.get("MANTID_LOCAL_DOCS_BASE", None)

    app = QApplication(sys.argv)
    show_help_page(relative_url=args.relative_url, local_docs=args.local_docs, online_base_url=args.online_base_url)

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
