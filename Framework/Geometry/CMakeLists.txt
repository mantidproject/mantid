set(SRC_FILES
    src/ComponentParser.cpp
    src/Crystal/BasicHKLFilters.cpp
    src/Crystal/BraggScatterer.cpp
    src/Crystal/BraggScattererFactory.cpp
    src/Crystal/BraggScattererInCrystalStructure.cpp
    src/Crystal/CenteringGroup.cpp
    src/Crystal/CompositeBraggScatterer.cpp
    src/Crystal/ConventionalCell.cpp
    src/Crystal/CrystalStructure.cpp
    src/Crystal/CyclicGroup.cpp
    src/Crystal/EdgePixel.cpp
    src/Crystal/Group.cpp
    src/Crystal/GroupTransformation.cpp
    src/Crystal/HKLFilter.cpp
    src/Crystal/HKLFilterWavelength.cpp
    src/Crystal/HKLGenerator.cpp
    src/Crystal/IndexingUtils.cpp
    src/Crystal/IsotropicAtomBraggScatterer.cpp
    src/Crystal/NiggliCell.cpp
    src/Crystal/OrientedLattice.cpp
    src/Crystal/PeakTransform.cpp
    src/Crystal/PeakTransformHKL.cpp
    src/Crystal/PeakTransformQLab.cpp
    src/Crystal/PeakTransformQSample.cpp
    src/Crystal/PeakTransformSelector.cpp
    src/Crystal/PointGroup.cpp
    src/Crystal/PointGroupFactory.cpp
    src/Crystal/ProductOfCyclicGroups.cpp
    src/Crystal/ReducedCell.cpp
    src/Crystal/ReflectionCondition.cpp
    src/Crystal/ReflectionGenerator.cpp
    src/Crystal/ScalarUtils.cpp
    src/Crystal/SpaceGroup.cpp
    src/Crystal/SpaceGroupFactory.cpp
    src/Crystal/StructureFactorCalculator.cpp
    src/Crystal/StructureFactorCalculatorSummation.cpp
    src/Crystal/SymmetryElement.cpp
    src/Crystal/SymmetryElementFactory.cpp
    src/Crystal/SymmetryOperation.cpp
    src/Crystal/SymmetryOperationFactory.cpp
    src/Crystal/SymmetryOperationSymbolParser.cpp
    src/Crystal/UnitCell.cpp
    src/Crystal/V3R.cpp
    src/DetermineGaugeVolume.cpp
    src/IObjComponent.cpp
    src/Instrument.cpp
    src/Instrument/CompAssembly.cpp
    src/Instrument/Component.cpp
    src/Instrument/ComponentHelper.cpp
    src/Instrument/ComponentInfo.cpp
    src/Instrument/ComponentInfoBankHelpers.cpp
    src/Instrument/ComponentInfoIterator.cpp
    src/Instrument/Container.cpp
    src/Instrument/Detector.cpp
    src/Instrument/DetectorGroup.cpp
    src/Instrument/DetectorInfo.cpp
    src/Instrument/FitParameter.cpp
    src/Instrument/Goniometer.cpp
    src/Instrument/GridDetector.cpp
    src/Instrument/GridDetectorPixel.cpp
    src/Instrument/IDFObject.cpp
    src/Instrument/InstrumentDefinitionParser.cpp
    src/Instrument/InstrumentVisitor.cpp
    src/Instrument/ObjCompAssembly.cpp
    src/Instrument/ObjComponent.cpp
    src/Instrument/ParComponentFactory.cpp
    src/Instrument/Parameter.cpp
    src/Instrument/ParameterMap.cpp
    src/Instrument/RectangularDetector.cpp
    src/Instrument/ReferenceFrame.cpp
    src/Instrument/SampleEnvironment.cpp
    src/Instrument/StructuredDetector.cpp
    src/Instrument/XMLInstrumentParameter.cpp
    src/MDGeometry/CompositeImplicitFunction.cpp
    src/MDGeometry/GeneralFrame.cpp
    src/MDGeometry/HKL.cpp
    src/MDGeometry/IMDDimension.cpp
    src/MDGeometry/IMDDimensionFactory.cpp
    src/MDGeometry/MDBoxImplicitFunction.cpp
    src/MDGeometry/MDFrameFactory.cpp
    src/MDGeometry/MDGeometryXMLBuilder.cpp
    src/MDGeometry/MDGeometryXMLParser.cpp
    src/MDGeometry/MDHistoDimension.cpp
    src/MDGeometry/MDHistoDimensionBuilder.cpp
    src/MDGeometry/MDImplicitFunction.cpp
    src/MDGeometry/MDPlane.cpp
    src/MDGeometry/MDPlaneImplicitFunction.cpp
    src/MDGeometry/NullImplicitFunction.cpp
    src/MDGeometry/QLab.cpp
    src/MDGeometry/QSample.cpp
    src/MDGeometry/UnknownFrame.cpp
    src/Math/Acomp.cpp
    src/Math/Algebra.cpp
    src/Math/BnId.cpp
    src/Math/ConvexPolygon.cpp
    src/Math/ConvexPolygonIterator.cpp
    src/Math/PolyBase.cpp
    src/Math/PolygonEdge.cpp
    src/Math/PolygonIntersection.cpp
    src/Math/Quadrilateral.cpp
    src/Math/RotCounter.cpp
    src/Math/Triple.cpp
    src/Math/mathSupport.cpp
    src/Objects/BoundingBox.cpp
    src/Objects/CSGObject.cpp
    src/Objects/InstrumentRayTracer.cpp
    src/Objects/MeshObject.cpp
    src/Objects/MeshObject2D.cpp
    src/Objects/MeshObjectCommon.cpp
    src/Objects/RuleItems.cpp
    src/Objects/Rules.cpp
    src/Objects/ShapeFactory.cpp
    src/Objects/Track.cpp
    src/RandomPoint.cpp
    src/Rasterize.cpp
    src/Rendering/GeometryHandler.cpp
    src/Rendering/GeometryTriangulator.cpp
    src/Rendering/ShapeInfo.cpp
    src/Rendering/vtkGeometryCacheReader.cpp
    src/Rendering/vtkGeometryCacheWriter.cpp
    src/Surfaces/Cone.cpp
    src/Surfaces/Cylinder.cpp
    src/Surfaces/General.cpp
    src/Surfaces/Line.cpp
    src/Surfaces/LineIntersectVisit.cpp
    src/Surfaces/Plane.cpp
    src/Surfaces/Quadratic.cpp
    src/Surfaces/Sphere.cpp
    src/Surfaces/Surface.cpp
    src/Surfaces/SurfaceFactory.cpp
    src/Surfaces/Torus.cpp
)

set(INC_FILES
    inc/MantidGeometry/ComponentParser.h
    inc/MantidGeometry/Crystal/BasicHKLFilters.h
    inc/MantidGeometry/Crystal/BraggScatterer.h
    inc/MantidGeometry/Crystal/BraggScattererFactory.h
    inc/MantidGeometry/Crystal/BraggScattererInCrystalStructure.h
    inc/MantidGeometry/Crystal/CenteringGroup.h
    inc/MantidGeometry/Crystal/CompositeBraggScatterer.h
    inc/MantidGeometry/Crystal/ConcretePeakTransformFactory.h
    inc/MantidGeometry/Crystal/ConventionalCell.h
    inc/MantidGeometry/Crystal/CrystalStructure.h
    inc/MantidGeometry/Crystal/CyclicGroup.h
    inc/MantidGeometry/Crystal/EdgePixel.h
    inc/MantidGeometry/Crystal/Group.h
    inc/MantidGeometry/Crystal/GroupTransformation.h
    inc/MantidGeometry/Crystal/HKLFilter.h
    inc/MantidGeometry/Crystal/HKLFilterWavelength.h
    inc/MantidGeometry/Crystal/HKLGenerator.h
    inc/MantidGeometry/Crystal/IPeak.h
    inc/MantidGeometry/Crystal/IndexingUtils.h
    inc/MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h
    inc/MantidGeometry/Crystal/MatrixVectorPair.h
    inc/MantidGeometry/Crystal/MatrixVectorPairParser.h
    inc/MantidGeometry/Crystal/NiggliCell.h
    inc/MantidGeometry/Crystal/OrientedLattice.h
    inc/MantidGeometry/Crystal/PeakShape.h
    inc/MantidGeometry/Crystal/PeakTransform.h
    inc/MantidGeometry/Crystal/PeakTransformFactory.h
    inc/MantidGeometry/Crystal/PeakTransformHKL.h
    inc/MantidGeometry/Crystal/PeakTransformQLab.h
    inc/MantidGeometry/Crystal/PeakTransformQSample.h
    inc/MantidGeometry/Crystal/PeakTransformSelector.h
    inc/MantidGeometry/Crystal/PointGroup.h
    inc/MantidGeometry/Crystal/PointGroupFactory.h
    inc/MantidGeometry/Crystal/ProductOfCyclicGroups.h
    inc/MantidGeometry/Crystal/ReducedCell.h
    inc/MantidGeometry/Crystal/ReflectionCondition.h
    inc/MantidGeometry/Crystal/ReflectionGenerator.h
    inc/MantidGeometry/Crystal/ScalarUtils.h
    inc/MantidGeometry/Crystal/SpaceGroup.h
    inc/MantidGeometry/Crystal/SpaceGroupFactory.h
    inc/MantidGeometry/Crystal/StructureFactorCalculator.h
    inc/MantidGeometry/Crystal/StructureFactorCalculatorSummation.h
    inc/MantidGeometry/Crystal/SymmetryElement.h
    inc/MantidGeometry/Crystal/SymmetryElementFactory.h
    inc/MantidGeometry/Crystal/SymmetryOperation.h
    inc/MantidGeometry/Crystal/SymmetryOperationFactory.h
    inc/MantidGeometry/Crystal/SymmetryOperationSymbolParser.h
    inc/MantidGeometry/Crystal/UnitCell.h
    inc/MantidGeometry/Crystal/V3R.h
    inc/MantidGeometry/DetermineGaugeVolume.h
    inc/MantidGeometry/ICompAssembly.h
    inc/MantidGeometry/IComponent.h
    inc/MantidGeometry/IDetector.h
    inc/MantidGeometry/IDetector_fwd.h
    inc/MantidGeometry/IObjComponent.h
    inc/MantidGeometry/Instrument.h
    inc/MantidGeometry/Instrument/CompAssembly.h
    inc/MantidGeometry/Instrument/Component.h
    inc/MantidGeometry/Instrument/ComponentHelper.h
    inc/MantidGeometry/Instrument/ComponentInfo.h
    inc/MantidGeometry/Instrument/ComponentInfoBankHelpers.h
    inc/MantidGeometry/Instrument/ComponentInfoItem.h
    inc/MantidGeometry/Instrument/ComponentInfoIterator.h
    inc/MantidGeometry/Instrument/ComponentVisitor.h
    inc/MantidGeometry/Instrument/Container.h
    inc/MantidGeometry/Instrument/Detector.h
    inc/MantidGeometry/Instrument/DetectorGroup.h
    inc/MantidGeometry/Instrument/DetectorInfo.h
    inc/MantidGeometry/Instrument/DetectorInfoItem.h
    inc/MantidGeometry/Instrument/DetectorInfoIterator.h
    inc/MantidGeometry/Instrument/FitParameter.h
    inc/MantidGeometry/Instrument/Goniometer.h
    inc/MantidGeometry/Instrument/GridDetector.h
    inc/MantidGeometry/Instrument/GridDetectorPixel.h
    inc/MantidGeometry/Instrument/IDFObject.h
    inc/MantidGeometry/Instrument/InfoIteratorBase.h
    inc/MantidGeometry/Instrument/InstrumentDefinitionParser.h
    inc/MantidGeometry/Instrument/InstrumentVisitor.h
    inc/MantidGeometry/Instrument/ObjCompAssembly.h
    inc/MantidGeometry/Instrument/ObjComponent.h
    inc/MantidGeometry/Instrument/ParComponentFactory.h
    inc/MantidGeometry/Instrument/Parameter.h
    inc/MantidGeometry/Instrument/ParameterFactory.h
    inc/MantidGeometry/Instrument/ParameterMap.h
    inc/MantidGeometry/Instrument/RectangularDetector.h
    inc/MantidGeometry/Instrument/ReferenceFrame.h
    inc/MantidGeometry/Instrument/SampleEnvironment.h
    inc/MantidGeometry/Instrument/SolidAngleParams.h
    inc/MantidGeometry/Instrument/StructuredDetector.h
    inc/MantidGeometry/Instrument/XMLInstrumentParameter.h
    inc/MantidGeometry/Instrument_fwd.h
    inc/MantidGeometry/MDGeometry/CompositeImplicitFunction.h
    inc/MantidGeometry/MDGeometry/GeneralFrame.h
    inc/MantidGeometry/MDGeometry/HKL.h
    inc/MantidGeometry/MDGeometry/IMDDimension.h
    inc/MantidGeometry/MDGeometry/IMDDimensionFactory.h
    inc/MantidGeometry/MDGeometry/MDBoxImplicitFunction.h
    inc/MantidGeometry/MDGeometry/MDDimensionExtents.h
    inc/MantidGeometry/MDGeometry/MDFrame.h
    inc/MantidGeometry/MDGeometry/MDFrameFactory.h
    inc/MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h
    inc/MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h
    inc/MantidGeometry/MDGeometry/MDGeometryXMLParser.h
    inc/MantidGeometry/MDGeometry/MDHistoDimension.h
    inc/MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h
    inc/MantidGeometry/MDGeometry/MDImplicitFunction.h
    inc/MantidGeometry/MDGeometry/MDPlane.h
    inc/MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h
    inc/MantidGeometry/MDGeometry/MDTypes.h
    inc/MantidGeometry/MDGeometry/MDWorkspaceConstants.h
    inc/MantidGeometry/MDGeometry/NullImplicitFunction.h
    inc/MantidGeometry/MDGeometry/QLab.h
    inc/MantidGeometry/MDGeometry/QSample.h
    inc/MantidGeometry/MDGeometry/UnknownFrame.h
    inc/MantidGeometry/Math/Acomp.h
    inc/MantidGeometry/Math/Algebra.h
    inc/MantidGeometry/Math/BnId.h
    inc/MantidGeometry/Math/ConvexPolygon.h
    inc/MantidGeometry/Math/MapSupport.h
    inc/MantidGeometry/Math/PolyBase.h
    inc/MantidGeometry/Math/PolygonEdge.h
    inc/MantidGeometry/Math/PolygonIntersection.h
    inc/MantidGeometry/Math/Quadrilateral.h
    inc/MantidGeometry/Math/RotCounter.h
    inc/MantidGeometry/Math/Triple.h
    inc/MantidGeometry/Math/mathSupport.h
    inc/MantidGeometry/Objects/BoundingBox.h
    inc/MantidGeometry/Objects/CSGObject.h
    inc/MantidGeometry/Objects/IObject.h
    inc/MantidGeometry/Objects/InstrumentRayTracer.h
    inc/MantidGeometry/Objects/MeshObject.h
    inc/MantidGeometry/Objects/MeshObject2D.h
    inc/MantidGeometry/Objects/MeshObjectCommon.h
    inc/MantidGeometry/Objects/Rules.h
    inc/MantidGeometry/Objects/ShapeFactory.h
    inc/MantidGeometry/Objects/Track.h
    inc/MantidGeometry/RandomPoint.h
    inc/MantidGeometry/Rasterize.h
    inc/MantidGeometry/Rendering/GeometryHandler.h
    inc/MantidGeometry/Rendering/GeometryTriangulator.h
    inc/MantidGeometry/Rendering/RenderingHelpers.h
    inc/MantidGeometry/Rendering/RenderingMesh.h
    inc/MantidGeometry/Rendering/ShapeInfo.h
    inc/MantidGeometry/Rendering/vtkGeometryCacheReader.h
    inc/MantidGeometry/Rendering/vtkGeometryCacheWriter.h
    inc/MantidGeometry/Surfaces/BaseVisit.h
    inc/MantidGeometry/Surfaces/Cone.h
    inc/MantidGeometry/Surfaces/Cylinder.h
    inc/MantidGeometry/Surfaces/General.h
    inc/MantidGeometry/Surfaces/Line.h
    inc/MantidGeometry/Surfaces/LineIntersectVisit.h
    inc/MantidGeometry/Surfaces/Plane.h
    inc/MantidGeometry/Surfaces/Quadratic.h
    inc/MantidGeometry/Surfaces/Sphere.h
    inc/MantidGeometry/Surfaces/Surface.h
    inc/MantidGeometry/Surfaces/SurfaceFactory.h
    inc/MantidGeometry/Surfaces/Torus.h
)

set(TEST_FILES
    AcompTest.h
    AlgebraTest.h
    BasicHKLFiltersTest.h
    BnIdTest.h
    BoundingBoxTest.h
    BraggScattererFactoryTest.h
    BraggScattererInCrystalStructureTest.h
    BraggScattererTest.h
    CSGObjectTest.h
    CenteringGroupTest.h
    CompAssemblyTest.h
    ComponentInfoBankHelpersTest.h
    ComponentInfoIteratorTest.h
    ComponentInfoTest.h
    ComponentParserTest.h
    ComponentTest.h
    CompositeBraggScattererTest.h
    CompositeImplicitFunctionTest.h
    ConeTest.h
    ContainerTest.h
    ConventionalCellTest.h
    ConvexPolygonIteratorTest.h
    ConvexPolygonTest.h
    CrystalStructureTest.h
    CyclicGroupTest.h
    CylinderTest.h
    DetectorGroupTest.h
    DetectorInfoIteratorTest.h
    DetectorTest.h
    DetermineGaugeVolumeTest.h
    FitParameterTest.h
    GeneralFrameTest.h
    GeneralTest.h
    GoniometerTest.h
    GridDetectorPixelTest.h
    GridDetectorTest.h
    GroupTest.h
    GroupTransformationTest.h
    HKLFilterTest.h
    HKLFilterWavelengthTest.h
    HKLGeneratorTest.h
    HKLTest.h
    IDFObjectTest.h
    IMDDimensionFactoryTest.h
    IMDDimensionTest.h
    IndexingUtilsTest.h
    InstrumentDefinitionParserTest.h
    InstrumentRayTracerTest.h
    InstrumentTest.h
    InstrumentVisitorTest.h
    IsotropicAtomBraggScattererTest.h
    LineIntersectVisitTest.h
    LineTest.h
    MDBoxImplicitFunctionTest.h
    MDFrameFactoryTest.h
    MDGeometryXMLParserTest.h
    MDHistoDimensionBuilderTest.h
    MDHistoDimensionTest.h
    MDImplicitFunctionTest.h
    MDPlaneImplicitFunctionTest.h
    MDPlaneTest.h
    MapSupportTest.h
    MathSupportTest.h
    MatrixVectorPairParserTest.h
    MatrixVectorPairTest.h
    MeshObject2DTest.h
    MeshObjectCommonTest.h
    MeshObjectTest.h
    NiggliCellTest.h
    NullImplicitFunctionTest.h
    ObjCompAssemblyTest.h
    ObjComponentTest.h
    OrientedLatticeTest.h
    ParCompAssemblyTest.h
    ParComponentFactoryTest.h
    ParDetectorTest.h
    ParInstrumentTest.h
    ParObjCompAssemblyTest.h
    ParObjComponentTest.h
    ParameterMapTest.h
    ParametrizedComponentTest.h
    PeakTransformHKLTest.h
    PeakTransformQLabTest.h
    PeakTransformQSampleTest.h
    PeakTransformSelectorTest.h
    PlaneTest.h
    PointGroupFactoryTest.h
    PointGroupTest.h
    PolygonEdgeTest.h
    PolygonIntersectionTest.h
    ProductOfCyclicGroupsTest.h
    QLabTest.h
    QSampleTest.h
    QuadrilateralTest.h
    RandomPointTest.h
    RasterizeTest.h
    RectangularDetectorTest.h
    ReducedCellTest.h
    ReferenceFrameTest.h
    ReflectionConditionTest.h
    ReflectionGeneratorTest.h
    RotCounterTest.h
    RulesBoolValueTest.h
    RulesCompGrpTest.h
    RulesCompObjTest.h
    RulesIntersectionTest.h
    RulesSurfPointTest.h
    RulesTest.h
    RulesUnionTest.h
    SampleEnvironmentTest.h
    ScalarUtilsTest.h
    ShapeFactoryTest.h
    ShapeInfoTest.h
    SpaceGroupFactoryTest.h
    SpaceGroupTest.h
    SphereTest.h
    StructureFactorCalculatorSummationTest.h
    StructureFactorCalculatorTest.h
    StructuredDetectorTest.h
    SurfaceFactoryTest.h
    SurfaceTest.h
    SymmetryElementFactoryTest.h
    SymmetryElementTest.h
    SymmetryOperationFactoryTest.h
    SymmetryOperationSymbolParserTest.h
    SymmetryOperationTest.h
    TorusTest.h
    TrackTest.h
    TripleTest.h
    UnitCellTest.h
    V3RTest.h
    XMLInstrumentParameterTest.h
)

set(GMOCK_TEST_FILES MDGeometryXMLBuilderTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# A few defines needed for OpenCascade on the Mac

if(APPLE)
  add_definitions(-DHAVE_IOSTREAM -DHAVE_LIMITS -DHAVE_IOMANIP)
endif()

if(ENABLE_OPENGL)
  list(APPEND INC_FILES inc/MantidGeometry/Rendering/OpenGL_Headers.h)
  list(APPEND SRC_FILES src/Rendering/RenderingHelpersOpenGL.cpp)
else()
  list(APPEND SRC_FILES src/Rendering/RenderingHelpersThrowing.cpp)
endif()

# Add a precompiled header where they are supported
enable_precompiled_headers(inc/MantidGeometry/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(Geometry ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Geometry ALIAS Geometry)

target_include_directories(
  Geometry PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(Geometry PROPERTIES OUTPUT_NAME MantidGeometry COMPILE_DEFINITIONS IN_MANTID_GEOMETRY)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Geometry PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Geometry PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

if(ENABLE_OPENCASCADE)
  target_include_directories(Geometry SYSTEM PRIVATE ${OPENCASCADE_INCLUDE_DIR})
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Geometry PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  Geometry
  PUBLIC Mantid::Kernel Mantid::Beamline MuParser::MuParser
  PRIVATE Mantid::Json
)

if(ENABLE_OPENGL)
  target_include_directories(Geometry SYSTEM PRIVATE ${OPENGL_INCLUDE_DIR})
  target_link_libraries(Geometry PRIVATE ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
endif()

if(ENABLE_OPENCASCADE)
  target_link_libraries(Geometry PRIVATE ${OPENCASCADE_LIBRARIES})
endif()

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Geometry PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Geometry> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Geometry TRUE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidGeometryTargets")
mtd_install_framework_lib(TARGETS Geometry INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
