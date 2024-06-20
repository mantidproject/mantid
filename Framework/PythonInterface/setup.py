# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import os
from setuptools import find_packages, setup


# The most basic setup possible to be able to use pip develop/install
setup(
    name="mantid",
    version=os.environ["MANTID_VERSION_STR"],
    packages=find_packages(exclude=["*.test", "plugins*"]),
    package_data={"": ["*.json", "*.ui"]},
)
