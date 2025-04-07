# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from setuptools import find_packages, setup


# The most basic setup possible to be able to use setup.py develop/install
setup(
    name="MantidWorkbench",  # probably the wrong name if someone wants to include it
    version=os.environ["MANTID_VERSION_STR"],
    install_requires=["mantidqtinterfaces", "instrumentview"],
    packages=find_packages(exclude=["*.test"]),
    package_data={"": ["*.ui"]},
    entry_points={"gui_scripts": ["workbench = workbench.app.main:main"]},
)
