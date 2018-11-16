# Small startup script that replaces the compiled executable from setuptools
# This script should be called with Mantid's Python from the launch_workbench run script
import workbench.app.mainwindow
workbench.app.mainwindow.main()
