# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Defines a set of aliases to make accessing certain objects easier,
like in mantid.api.
"""

from mantid.kernel._aliases import lazy_instance_access
from mantid.geometry import SpaceGroupFactoryImpl, SymmetryOperationFactoryImpl, SymmetryElementFactoryImpl, PointGroupFactoryImpl

###############################################################################
# Singletons
###############################################################################
SpaceGroupFactory = lazy_instance_access(SpaceGroupFactoryImpl)
SymmetryOperationFactory = lazy_instance_access(SymmetryOperationFactoryImpl)
SymmetryElementFactory = lazy_instance_access(SymmetryElementFactoryImpl)
PointGroupFactory = lazy_instance_access(PointGroupFactoryImpl)
