"""
    Defines a set of aliases to make accessing certain objects easier
"""
from _api import (FrameworkManagerImpl, AnalysisDataServiceImpl, 
                  AlgorithmFactoryImpl, AlgorithmManagerImpl,
                  FileFinderImpl, WorkspaceFactoryImpl)

###############################################################################
# Singleton
###############################################################################

FrameworkManager = FrameworkManagerImpl.Instance()

AnalysisDataService = AnalysisDataServiceImpl.Instance()
mtd = AnalysisDataService #tradition

AlgorithmFactory = AlgorithmFactoryImpl.Instance()

AlgorithmManager = AlgorithmManagerImpl.Instance()

FileFinder = FileFinderImpl.Instance()

WorkspaceFactory = WorkspaceFactoryImpl.Instance()