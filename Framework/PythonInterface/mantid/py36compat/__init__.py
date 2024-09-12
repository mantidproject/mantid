# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


"""
mantid.py36compat

Provides a compatibility layer to backport features found in later
Python versions (3.7 onwards) to Ubuntu 18.04 / RHEL 7
which are both on Python 3.6.
"""

import sys

__requires_compat = False if sys.version_info[0:2] > (3, 6) else True

if __requires_compat:
    from ._dataclasses.dataclasses import dataclass, field
else:
    from dataclasses import dataclass, field  # noqa: F401

__all__ = "dataclass, field"
