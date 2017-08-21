"""
Plugins
=======

Defines Python objects that wrap the C++ DataObjects namespace. These are exported
so that boost::python can automagically downcast objects to the correct leaf type.

For example, sunclasses of composite function such as ProductFunction is made 
available as ProductFunction rather than IFunction and so methods, such as
__len__() & add() inherited from CompositeFunction are made available.

The names from the library are not imported by default as it is best if the interface classes
are used for checks such as isinstance()
"""
from __future__ import (absolute_import, division,
                        print_function)

###############################################################################
# Load the C++ library and register the C++ class exports
###############################################################################
from . import _curvefitting
