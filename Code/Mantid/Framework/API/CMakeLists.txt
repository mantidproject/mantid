set ( SRC_FILES src/Algorithm.cpp src/AlgorithmFactory.cpp
      src/AlgorithmHistory.cpp src/AlgorithmManager.cpp
      src/AlgorithmProxy.cpp src/AnalysisDataService.cpp
      src/ArchiveSearchFactory.cpp src/Axis.cpp src/CatalogFactory.cpp
      src/CloneableAlgorithm.cpp src/Column.cpp src/ColumnFactory.cpp
      src/CompositeFunction.cpp src/ConstraintFactory.cpp
      src/Expression.cpp src/FileFinder.cpp src/FileProperty.cpp
      src/FrameworkManager.cpp src/Function.cpp
      src/FunctionFactory.cpp src/IEventWorkspace.cpp
      src/IFunction.cpp src/PairedGroupAlgorithm.cpp
      src/NumericAxis.cpp src/MemoryManager.cpp
      src/MatrixWorkspace.cpp src/LocatedDataValue.cpp
      src/LocatedDataRef.cpp
      src/ImplicitFunctionParameterParserFactory.cpp
      src/ITableWorkspace.cpp src/IPeakFunction.cpp
      src/InstrumentDataService.cpp src/ImplictFunctionFactory.cpp
      src/ImplicitFunctionParserFactory.cpp src/ParameterReference.cpp
      src/ParameterTie.cpp src/Progress.cpp src/RefAxis.cpp
      src/Run.cpp src/Sample.cpp src/SpectraAxis.cpp
      src/SpectraDetectorMap.cpp src/TableRow.cpp src/TextAxis.cpp
      src/Workspace.cpp src/WorkspaceFactory.cpp
      src/WorkspaceGroup.cpp src/WorkspaceHistory.cpp
      src/WorkspaceOpOverloads.cpp src/WorkspaceProperty.cpp
      src/WorkspaceTracer.cpp )

set ( INC_FILES inc/MantidAPI/AlgorithmFactory.h
      inc/MantidAPI/Algorithm.h inc/MantidAPI/AlgorithmHistory.h
      inc/MantidAPI/AlgorithmManager.h
      inc/MantidAPI/AlgorithmObserver.h inc/MantidAPI/AlgorithmProxy.h
      inc/MantidAPI/AnalysisDataService.h
      inc/MantidAPI/ArchiveSearchFactory.h inc/MantidAPI/Axis.h
      inc/MantidAPI/CatalogFactory.h
      inc/MantidAPI/CloneableAlgorithm.h inc/MantidAPI/ColumnFactory.h
      inc/MantidAPI/Column.h inc/MantidAPI/CompositeFunction.h
      inc/MantidAPI/ConstraintFactory.h inc/MantidAPI/DeclareUserAlg.h
      inc/MantidAPI/DllExport.h inc/MantidAPI/Expression.h
      inc/MantidAPI/FileFinder.h inc/MantidAPI/FileProperty.h
      inc/MantidAPI/FrameworkManager.h inc/MantidAPI/FunctionFactory.h
      inc/MantidAPI/Function.h inc/MantidAPI/IAlgorithm.h
      inc/MantidAPI/IArchiveSearch.h
      inc/MantidAPI/IBackgroundFunction.h inc/MantidAPI/ICatalog.h
      inc/MantidAPI/IConstraint.h inc/MantidAPI/IDataItem.h
      inc/MantidAPI/IEventWorkspace.h inc/MantidAPI/IFunction.h
      inc/MantidAPI/IFunctionWithLocation.h
      inc/MantidAPI/ILocatedData.h inc/MantidAPI/IMDWorkspace.h
      inc/MantidAPI/ImplicitFunctionBuilder.h
      inc/MantidAPI/ImplicitFunctionFactory.h
      inc/MantidAPI/ImplicitFunction.h
      inc/MantidAPI/ImplicitFunctionParameter.h
      inc/MantidAPI/ImplicitFunctionParameterParserFactory.h
      inc/MantidAPI/ImplicitFunctionParameterParser.h
      inc/MantidAPI/ImplicitFunctionParserFactory.h
      inc/MantidAPI/ImplicitFunctionParser.h
      inc/MantidAPI/InstrumentDataService.h
      inc/MantidAPI/IPeakFunction.h inc/MantidAPI/ITableWorkspace.h
      inc/MantidAPI/IWorkspaceProperty.h
      inc/MantidAPI/LocatedDataRef.h inc/MantidAPI/LocatedDataValue.h
      inc/MantidAPI/MatrixWorkspace.h inc/MantidAPI/MemoryManager.h
      inc/MantidAPI/NumericAxis.h inc/MantidAPI/PairedGroupAlgorithm.h
      inc/MantidAPI/ParameterReference.h inc/MantidAPI/ParameterTie.h
      inc/MantidAPI/Point3D.h inc/MantidAPI/Progress.h
      inc/MantidAPI/RefAxis.h inc/MantidAPI/Run.h
      inc/MantidAPI/Sample.h inc/MantidAPI/SpectraAxis.h
      inc/MantidAPI/SpectraDetectorMap.h inc/MantidAPI/TableRow.h
      inc/MantidAPI/TextAxis.h inc/MantidAPI/WorkspaceFactory.h
      inc/MantidAPI/WorkspaceGroup.h inc/MantidAPI/Workspace.h
      inc/MantidAPI/WorkspaceHistory.h
      inc/MantidAPI/WorkspaceIteratorCode.h
      inc/MantidAPI/WorkspaceIterator.h
      inc/MantidAPI/WorkspaceOpOverloads.h
      inc/MantidAPI/WorkspaceProperty.h
      inc/MantidAPI/WorkspaceTracer.h
      inc/MantidAPI/WorkspaceValidators.h )

# For Windows:
add_definitions ( -DIN_MANTID_API )

# Have to link to winsock library on Windows
if ( WIN32 )
  set ( WINSOCK ws2_32 )
endif ()

# Add the target for this directory
add_library ( API ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( API PROPERTIES OUTPUT_NAME MantidAPI )
# Add to the 'Framework' group in VS
set_property ( TARGET API PROPERTY FOLDER "Framework" )

target_link_libraries ( API ${MANTIDLIBS} ${WINSOCK} )
