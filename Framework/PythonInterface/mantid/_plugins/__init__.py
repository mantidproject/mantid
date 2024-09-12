# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Plugins
=======

Defines Python objects that wrap classes from the framework defined as plugins.
The classes defined here simply export the class types to Python, but not any additional functionality.
This allows Boost Python to automatically downcast a type to the actual leaf type,
making it easier to work with these objects in Python

For example, ProductFunction is made available as ProductFunction rather than IFunction
and so methods, such as __len__() & add() inherited from CompositeFunction are made available.

The names from the library are not imported by default as it is best if the interface classes
are used for checks such as isinstance()
"""

###############################################################################
# Load the C++ library and register the C++ class exports
###############################################################################
from mantid.utils import import_mantid_cext

import_mantid_cext("._curvefitting", "mantid._plugins", globals())
