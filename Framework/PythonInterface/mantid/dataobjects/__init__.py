# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
datobjects
==========

Defines Python objects that wrap the C++ DataObjects namespace. These are exported
so that boost::python can automagically downcast objects to the correct leaf type.

For example, AnalysisDataService.retrieve() returns a pointer to a basic Workspace
object. In C++ a dynamic_cast is used to get the correct type but Python has no concept
of this so users would be left with a pretty useless object.Boost::python can automatically
downcast to the correct leaf type if the export for that class exists in the registry.

The names from the library are not imported by default as it is best if the interface classes
are used for checks such as isinstance()
"""
from __future__ import absolute_import

###############################################################################
# Load the C++ library and register the C++ class exports
###############################################################################
from mantid.utils import _shared_cextension, import_mantid

with _shared_cextension():
    _dataobjects = import_mantid('._dataobjects', 'mantid.dataobjects', globals())
