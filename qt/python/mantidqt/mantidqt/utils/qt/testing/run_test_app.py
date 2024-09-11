# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""A small command line utility that can be used to manually test a widget in isolation.

Usage:

    python run_test_app.py qualified.name.of.SomeWidget [--script qualified.name.of.some_script]

The widget (and optional script) should be in the location form where python could import it as in:

    form qualified.name.of import SomeWidget

The widget is created by calling SomeWidget() constructor without arguments. If __init__ has arguments or
the widget needs to be specially prepared any function returning an instance of SomeWidget can be used instead.

Optional --script argument expects a name of a function which takes a widget as its single argument.
If specified the script is imported and run after the widget is created.

"""

import argparse
import sys
from gui_test_runner import open_in_window

parser = argparse.ArgumentParser()
parser.add_argument(
    "widget",
    help="A qualified name of a widget to open for testing. The name must contain the "
    "python module where the widget is defined, eg mypackage.mymodule.MyWidget",
)
parser.add_argument(
    "--script",
    help="A qualified name of a python function to run to test the widget."
    " The function must take a single argument - the widget."
    " The name must contain the python module where the function is defined,"
    " eg somepackage.somemodule.test_my_widget",
)
args = parser.parse_args()
sys.exit(open_in_window(args.widget, args.script, is_cli=True))
