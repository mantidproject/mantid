# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from setuptools import find_packages, setup

setup(
    name="instrumentview",
    install_requires=["mantidqt"],
    version=os.environ["MANTID_VERSION_STR"],
    entry_points={"gui_scripts": ["instrumentview = InstrumentView:main"]},
    packages=find_packages(exclude=["*.test"]),
)
