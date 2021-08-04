# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from setuptools import find_packages, setup

setup(
    name='mantidqt',  # must match what is required by workbench setup.py
    install_requires=['mantid'],
    version=os.environ['MANTID_VERSION_STR'],
    packages=find_packages(exclude=['*.test']),
    package_data={'': ['*.ui']},
)
