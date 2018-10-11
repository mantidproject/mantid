# Small startup script that replaces the compiled executable from setuptools
# This script should be called with Mantid's Python from the launch_workbench run script
import os

script_root_directory = os.path.dirname(os.path.realpath(__file__))

# Appends the Mantid install/bin directory to the PATH so that the import can find the QT DLLs
os.environ["PATH"] += ";{0}".format(script_root_directory)

import workbench.app.mainwindow
workbench.app.mainwindow.main()
