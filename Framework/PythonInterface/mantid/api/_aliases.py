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
from ..kernel._aliases import create_instance_holder

# Historically the singleton aliases mapped to the instances rather than
# the class types, i.e. AnalysisDataService is the instance and not the type,
# which doesn't match the C++ behaviour.
AnalysisDataService = create_instance_holder(AnalysisDataServiceImpl)
AlgorithmFactory = create_instance_holder(AlgorithmFactoryImpl)
AlgorithmManager = create_instance_holder(AlgorithmManagerImpl)
FileFinder = create_instance_holder(FileFinderImpl)
FileLoaderRegistry = create_instance_holder(FileLoaderRegistryImpl)
FrameworkManager = create_instance_holder(FrameworkManagerImpl)
FunctionFactory = create_instance_holder(FunctionFactoryImpl)
WorkspaceFactory = create_instance_holder(WorkspaceFactoryImpl)
CatalogManager = create_instance_holder(CatalogManagerImpl)

# backwards-compatible
mtd = AnalysisDataService
