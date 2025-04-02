# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys

from qtpy.QtWidgets import QApplication
from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter

_presenter = None


def show_help_page(relativeUrl, localDocs=None, onlineBaseUrl="https://docs.mantidproject.org/"):
    """
    Show the help window at the given relative URL path.
    """
    global _presenter
    if _presenter is None:
        # Create a Presenter once. Re-use it on subsequent calls.
        _presenter = HelpWindowPresenter(localDocs=localDocs, onlineBaseUrl=onlineBaseUrl)

    # Ask the Presenter to load the requested page
    _presenter.show_help_page(relativeUrl)


def main(cmdargs=sys.argv):
    """
    Run this script standalone to test the Python-based Help Window.
    """
    import argparse

    parser = argparse.ArgumentParser(description="Standalone test of the Python-based Mantid Help Window.")
    parser.add_argument(
        "relativeUrl", nargs="?", default="", help="Relative doc path (e.g. 'algorithms/Load-v1.html'), defaults to 'index.html' if empty."
    )
    parser.add_argument("--local-docs", default=None, help="Path to local Mantid HTML docs. Overrides environment if set.")
    parser.add_argument(
        "--online-base-url",
        default="https://docs.mantidproject.org/",
        help="Base URL for online docs if local docs are not set or invalid.",
    )
    args = parser.parse_args(cmdargs or sys.argv[1:])

    # If user gave no --local-docs, fall back to environment
    if args.local_docs is None:
        args.local_docs = os.environ.get("MANTID_LOCAL_DOCS_BASE", None)

    app = QApplication(sys.argv)

    # Show the requested help page
    show_help_page(relativeUrl=args.relativeUrl, localDocs=args.local_docs, onlineBaseUrl=args.online_base_url)

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
