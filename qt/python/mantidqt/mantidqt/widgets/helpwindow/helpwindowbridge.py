# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
from mantidqt.widgets.helpwindow.helpwindowpresenter import HelpWindowPresenter
from qtpy.QtWidgets import QApplication

_presenter = None


def show_help_page(relative_url, local_docs=None, online_base_url="https://docs.mantidproject.org/"):
    """
    Show the help window at the given relative URL path.
    """
    global _presenter
    if _presenter is None:
        # Create a Presenter once. Re-use it on subsequent calls.
        _presenter = HelpWindowPresenter(local_docs=local_docs, online_base_url=online_base_url)

    # Ask the Presenter to load the requested page
    _presenter.showHelpPage(relative_url)


def main(cmdargs=None):
    """
    Run this script standalone to test the Python-based Help Window.

    Example:
      python helpwindowbridge.py algorithms/Load-v1.html \
          --local-docs /path/to/build/docs/html
    """
    import argparse

    parser = argparse.ArgumentParser(description="Standalone test of the Python-based Mantid Help Window.")
    parser.add_argument(
        "relative_url", nargs="?", default="", help="Relative doc path (e.g. 'algorithms/Load-v1.html'), defaults to 'index.html' if empty."
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
    show_help_page(relative_url=args.relative_url, local_docs=args.local_docs, online_base_url=args.online_base_url)

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
