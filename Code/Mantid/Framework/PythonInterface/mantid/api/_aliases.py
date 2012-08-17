"""
    Defines a set of aliases to make accessing certain objects easier
"""
from _api import (FrameworkManagerImpl, AnalysisDataServiceImpl, 
                  AlgorithmFactoryImpl, AlgorithmManagerImpl,
                  FileFinderImpl, WorkspaceFactoryImpl, 
                  PropertyManagerDataServiceImpl)

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

PropertyManagerDataService = PropertyManagerDataServiceImpl.Instance()
pmds = PropertyManagerDataService