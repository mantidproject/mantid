"""
    Defines a set of aliases to make accessing certain objects easier,
    like in mantid.api.
"""
from _geometry import (SpaceGroupFactoryImpl,
                  SymmetryOperationFactoryImpl,
                  SymmetryElementFactoryImpl,
                  PointGroupFactoryImpl)

###############################################################################
# Singleton
###############################################################################

SpaceGroupFactory = SpaceGroupFactoryImpl.Instance()

SymmetryOperationFactory = SymmetryOperationFactoryImpl.Instance()

SymmetryElementFactory = SymmetryElementFactoryImpl.Instance()

PointGroupFactory = PointGroupFactoryImpl.Instance()