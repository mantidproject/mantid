# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Defines a set of aliases to make accessing certain objects easier
"""
from __future__ import (absolute_import, division,
                        print_function)

from ._api import (FrameworkManagerImpl, AnalysisDataServiceImpl,
                   AlgorithmFactoryImpl, AlgorithmManagerImpl,
                   FileFinderImpl, FileLoaderRegistryImpl, FunctionFactoryImpl,
                   WorkspaceFactoryImpl, CatalogManagerImpl)

###############################################################################
# Singleton
###############################################################################

FrameworkManager = FrameworkManagerImpl.Instance()
AnalysisDataService = AnalysisDataServiceImpl.Instance()
mtd = AnalysisDataService #tradition
AlgorithmFactory = AlgorithmFactoryImpl.Instance()
AlgorithmManager = AlgorithmManagerImpl.Instance()
FileFinder = FileFinderImpl.Instance()
FileLoaderRegistry = FileLoaderRegistryImpl.Instance()
FunctionFactory = FunctionFactoryImpl.Instance()
WorkspaceFactory = WorkspaceFactoryImpl.Instance()
CatalogManager = CatalogManagerImpl.Instance()
