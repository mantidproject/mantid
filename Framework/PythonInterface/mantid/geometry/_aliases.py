# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Defines a set of aliases to make accessing certain objects easier,
    like in mantid.api.
"""
from __future__ import (absolute_import, division,
                        print_function)

from ._geometry import (SpaceGroupFactoryImpl, SymmetryOperationFactoryImpl,
                        SymmetryElementFactoryImpl, PointGroupFactoryImpl)

###############################################################################
# Singleton
###############################################################################
SpaceGroupFactory = SpaceGroupFactoryImpl.Instance()
SymmetryOperationFactory = SymmetryOperationFactoryImpl.Instance()
SymmetryElementFactory = SymmetryElementFactoryImpl.Instance()
PointGroupFactory = PointGroupFactoryImpl.Instance()
