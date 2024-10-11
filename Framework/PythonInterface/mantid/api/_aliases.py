# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Defines a set of aliases to make accessing certain objects easier
"""

import sys

from mantid.api import (
    AlgorithmFactoryImpl,
    AlgorithmManagerImpl,
    AnalysisDataServiceImpl,
    CatalogManagerImpl,
    FileFinderImpl,
    FileLoaderRegistryImpl,
    FrameworkManagerImpl,
    FunctionFactoryImpl,
    WorkspaceFactoryImpl,
)
from mantid.kernel._aliases import lazy_instance_access

# Historically the singleton aliases mapped to the instances rather than
# the class types, i.e. AnalysisDataService is the instance and not the type,
# which doesn't match the C++ behaviour.
#
# Exit handlers are important in some cases as the associated singleton
# stores references to python objects that need to be cleaned up
# Without a python-based exit handler the singletons are only cleaned
# up after main() and this is too late to acquire the GIL to be able to
# delete the python objects.
# If you see a segfault late in a python process related to the GIL
# it is likely an exit handler is missing.
AnalysisDataService = lazy_instance_access(AnalysisDataServiceImpl, key_as_str=True)
AlgorithmFactory = lazy_instance_access(AlgorithmFactoryImpl)
AlgorithmManager = lazy_instance_access(AlgorithmManagerImpl)
FileFinder = lazy_instance_access(FileFinderImpl)
FileLoaderRegistry = lazy_instance_access(FileLoaderRegistryImpl)
FrameworkManager = lazy_instance_access(FrameworkManagerImpl)
FunctionFactory = lazy_instance_access(FunctionFactoryImpl)
WorkspaceFactory = lazy_instance_access(WorkspaceFactoryImpl)
CatalogManager = lazy_instance_access(CatalogManagerImpl)
if sys.platform.startswith("linux"):
    from mantid.api import AlgoTimeRegisterImpl

    AlgoTimeRegister = lazy_instance_access(AlgoTimeRegisterImpl)

# backwards-compatible
mtd = AnalysisDataService
