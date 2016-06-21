"""
    Defines a set of aliases to make accessing certain objects easier
"""
from _api import (FrameworkManagerImpl, AnalysisDataServiceImpl,
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
