# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys

from qtpy.QtWidgets import QApplication
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


def main(cmdargs=sys.argv):
    """
    Run this script standalone to test the Python-based Help Window.
    Local docs path is determined from Mantid's ConfigService.
    """
    import argparse

    parser = argparse.ArgumentParser(description="Standalone test of the Python-based Mantid Help Window.")
    parser.add_argument(
        "relativeUrl", nargs="?", default="", help="Relative doc path (e.g. 'algorithms/Load-v1.html'), defaults to 'index.html' if empty."
    )

    parser.add_argument(
        "--online-base-url",
        default="https://docs.mantidproject.org/",
        help="Base URL for online docs if local docs path from config is invalid or not found.",
    )
    args = parser.parse_args(cmdargs or sys.argv[1:])

    try:
        import mantid.kernel

        log = mantid.kernel.Logger("HelpWindowBridge")
        log.information("Mantid kernel imported successfully.")
    except ImportError as e:
        print(f"ERROR: Failed to import Mantid Kernel: {e}", file=sys.stderr)
        print(
            "Ensure Mantid is built and PYTHONPATH is set correctly (e.g., export PYTHONPATH=/path/to/mantid/build/bin:$PYTHONPATH)",
            file=sys.stderr,
        )
        sys.exit(1)

    app = QApplication(sys.argv)

    show_help_page(relativeUrl=args.relativeUrl, onlineBaseUrl=args.online_base_url)

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
