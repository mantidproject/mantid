set ( SRC_FILES
	${SRC_FILES} ${OPENCASCADE_SRC}
	src/ComponentParser.cpp
	src/Crystal/ConventionalCell.cpp
	src/Crystal/IndexingUtils.cpp
	src/Crystal/OrientedLattice.cpp
	src/Crystal/PointGroup.cpp
	src/Crystal/ReducedCell.cpp
	src/Crystal/ReflectionCondition.cpp
	src/Crystal/ScalarUtils.cpp
	src/Crystal/UnitCell.cpp
	src/IObjComponent.cpp
	src/Instrument.cpp
	src/Instrument/CompAssembly.cpp
	src/Instrument/Component.cpp
	src/Instrument/Detector.cpp
	src/Instrument/DetectorGroup.cpp
	src/Instrument/FitParameter.cpp
	src/Instrument/Goniometer.cpp
	src/Instrument/InstrumentDefinitionParser.cpp
	src/Instrument/NearestNeighbours.cpp
	src/Instrument/NearestNeighboursFactory.cpp
	src/Instrument/ObjCompAssembly.cpp
	src/Instrument/ObjComponent.cpp
	src/Instrument/OneToOneSpectraDetectorMap.cpp
	src/Instrument/ParComponentFactory.cpp
	src/Instrument/Parameter.cpp
	src/Instrument/ParameterMap.cpp
	src/Instrument/RectangularDetector.cpp
	src/Instrument/RectangularDetectorPixel.cpp
	src/Instrument/ReferenceFrame.cpp
	src/Instrument/XMLlogfile.cpp
	src/MDGeometry/CompositeImplicitFunction.cpp
	src/MDGeometry/IMDDimension.cpp
	src/MDGeometry/IMDDimensionFactory.cpp
	src/MDGeometry/MDBoxImplicitFunction.cpp
	src/MDGeometry/MDGeometryXMLBuilder.cpp
	src/MDGeometry/MDGeometryXMLParser.cpp
	src/MDGeometry/MDHistoDimension.cpp
	src/MDGeometry/MDHistoDimensionBuilder.cpp
	src/MDGeometry/MDImplicitFunction.cpp
	src/MDGeometry/MDPlane.cpp
	src/MDGeometry/MDPlaneImplicitFunction.cpp
	src/MDGeometry/NullImplicitFunction.cpp
	src/Math/Acomp.cpp
	src/Math/Algebra.cpp
	src/Math/BnId.cpp
	src/Math/ConvexPolygon.cpp
	src/Math/LaszloIntersection.cpp
	src/Math/PolyBase.cpp
	src/Math/PolygonEdge.cpp
	src/Math/Quadrilateral.cpp
	src/Math/RotCounter.cpp
	src/Math/Triple.cpp
	src/Math/Vertex2D.cpp
	src/Math/Vertex2DList.cpp
	src/Math/mathSupport.cpp
	src/Objects/BoundingBox.cpp
	src/Objects/InstrumentRayTracer.cpp
	src/Objects/Material.cpp
	src/Objects/Object.cpp
	src/Objects/RuleItems.cpp
	src/Objects/Rules.cpp
	src/Objects/ShapeFactory.cpp
	src/Objects/Track.cpp
	src/Rendering/BitmapGeometryHandler.cpp
	src/Rendering/CacheGeometryGenerator.cpp
	src/Rendering/CacheGeometryHandler.cpp
	src/Rendering/CacheGeometryRenderer.cpp
	src/Rendering/GeometryHandler.cpp
	src/Rendering/GluGeometryHandler.cpp
	src/Rendering/GluGeometryRenderer.cpp
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

set ( OPENCASCADE_SRC
	src/Rendering/OCGeometryGenerator.cpp
	src/Rendering/OCGeometryHandler.cpp
	src/Rendering/OCGeometryRenderer.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/Instrument/CompAssembly.cpp
        src/Instrument/ObjCompAssembly.cpp 
        src/Rendering/OCGeometryHandler.cpp
        src/Rendering/OCGeometryRenderer.cpp
        )
        
set ( INC_FILES
	inc/MantidGeometry/ComponentParser.h
	inc/MantidGeometry/Crystal/ConventionalCell.h
	inc/MantidGeometry/Crystal/IndexingUtils.h
	inc/MantidGeometry/Crystal/OrientedLattice.h
	inc/MantidGeometry/Crystal/PointGroup.h
	inc/MantidGeometry/Crystal/ReducedCell.h
	inc/MantidGeometry/Crystal/ReflectionCondition.h
	inc/MantidGeometry/Crystal/ScalarUtils.h
	inc/MantidGeometry/Crystal/UnitCell.h
	inc/MantidGeometry/DllConfig.h
	inc/MantidGeometry/ICompAssembly.h
	inc/MantidGeometry/IComponent.h
	inc/MantidGeometry/IDetector.h
	inc/MantidGeometry/IObjComponent.h
	inc/MantidGeometry/ISpectraDetectorMap.h
	inc/MantidGeometry/Instrument.h
	inc/MantidGeometry/Instrument/CompAssembly.h
	inc/MantidGeometry/Instrument/Component.h
	inc/MantidGeometry/Instrument/Detector.h
	inc/MantidGeometry/Instrument/DetectorGroup.h
	inc/MantidGeometry/Instrument/FitParameter.h
	inc/MantidGeometry/Instrument/Goniometer.h
	inc/MantidGeometry/Instrument/INearestNeighbours.h
	inc/MantidGeometry/Instrument/INearestNeighboursFactory.h
	inc/MantidGeometry/Instrument/InstrumentDefinitionParser.h
	inc/MantidGeometry/Instrument/NearestNeighbours.h
	inc/MantidGeometry/Instrument/NearestNeighboursFactory.h
	inc/MantidGeometry/Instrument/ObjCompAssembly.h
	inc/MantidGeometry/Instrument/ObjComponent.h
	inc/MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h
	inc/MantidGeometry/Instrument/ParComponentFactory.h
	inc/MantidGeometry/Instrument/Parameter.h
	inc/MantidGeometry/Instrument/ParameterFactory.h
	inc/MantidGeometry/Instrument/ParameterMap.h
	inc/MantidGeometry/Instrument/RectangularDetector.h
	inc/MantidGeometry/Instrument/RectangularDetectorPixel.h
	inc/MantidGeometry/Instrument/ReferenceFrame.h
	inc/MantidGeometry/Instrument/XMLlogfile.h
	inc/MantidGeometry/MDGeometry/CompositeImplicitFunction.h
	inc/MantidGeometry/MDGeometry/IMDDimension.h
	inc/MantidGeometry/MDGeometry/IMDDimensionFactory.h
	inc/MantidGeometry/MDGeometry/MDBoxImplicitFunction.h
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
	inc/MantidGeometry/Math/Acomp.h
	inc/MantidGeometry/Math/Algebra.h
	inc/MantidGeometry/Math/BnId.h
	inc/MantidGeometry/Math/ConvexPolygon.h
	inc/MantidGeometry/Math/LaszloIntersection.h
	inc/MantidGeometry/Math/MapSupport.h
	inc/MantidGeometry/Math/PolyBase.h
	inc/MantidGeometry/Math/PolygonEdge.h
	inc/MantidGeometry/Math/Quadrilateral.h
	inc/MantidGeometry/Math/RotCounter.h
	inc/MantidGeometry/Math/Triple.h
	inc/MantidGeometry/Math/Vertex2D.h
	inc/MantidGeometry/Math/Vertex2DList.h
	inc/MantidGeometry/Math/mathSupport.h
	inc/MantidGeometry/Objects/BoundingBox.h
	inc/MantidGeometry/Objects/InstrumentRayTracer.h
	inc/MantidGeometry/Objects/Material.h
	inc/MantidGeometry/Objects/Object.h
	inc/MantidGeometry/Objects/Rules.h
	inc/MantidGeometry/Objects/ShapeFactory.h
	inc/MantidGeometry/Objects/Track.h
	inc/MantidGeometry/Rendering/BitmapGeometryHandler.h
	inc/MantidGeometry/Rendering/CacheGeometryGenerator.h
	inc/MantidGeometry/Rendering/CacheGeometryHandler.h
	inc/MantidGeometry/Rendering/CacheGeometryRenderer.h
	inc/MantidGeometry/Rendering/GeometryHandler.h
	inc/MantidGeometry/Rendering/GluGeometryHandler.h
	inc/MantidGeometry/Rendering/GluGeometryRenderer.h
	inc/MantidGeometry/Rendering/OCGeometryGenerator.h
	inc/MantidGeometry/Rendering/OCGeometryHandler.h
	inc/MantidGeometry/Rendering/OCGeometryRenderer.h
	inc/MantidGeometry/Rendering/OpenCascadeConfig.h
	inc/MantidGeometry/Rendering/OpenGL_Headers.h
	inc/MantidGeometry/Rendering/vtkGeometryCacheReader.h
	inc/MantidGeometry/Rendering/vtkGeometryCacheWriter.h
	inc/MantidGeometry/Surfaces/BaseVisit.h
)

set ( TEST_FILES
	test/AcompTest.h
	test/AlgebraTest.h
	test/BnIdTest.h
	test/BoundingBoxTest.h
	test/CompAssemblyTest.h
	test/ComponentParserTest.h
	test/ComponentTest.h
	test/CompositeImplicitFunctionTest.h
	test/ConeTest.h
	test/ConventionalCellTest.h
	test/ConvexPolygonTest.h
	test/CylinderTest.h
	test/DetectorGroupTest.h
	test/DetectorTest.h
	test/FitParameterTest.h
	test/GeneralTest.h
	test/GoniometerTest.h
	test/IMDDimensionFactoryTest.h
	test/IMDDimensionTest.h
	test/IndexingUtilsTest.h
	test/InstrumentDefinitionParserTest.h
	test/InstrumentRayTracerTest.h
	test/InstrumentTest.h
	test/LaszloIntersectionTest.h
	test/LineIntersectVisitTest.h
	test/LineTest.h
	test/MDBoxImplicitFunctionTest.h
	test/MDGeometryXMLParserTest.h
	test/MDHistoDimensionBuilderTest.h
	test/MDHistoDimensionTest.h
	test/MDImplicitFunctionTest.h
	test/MDPlaneImplicitFunctionTest.h
	test/MDPlaneTest.h
	test/MapSupportTest.h
	test/MaterialTest.h
	test/MathSupportTest.h
	test/NearestNeighboursFactoryTest.h
	test/NearestNeighboursTest.h
	test/NullImplicitFunctionTest.h
	test/ObjCompAssemblyTest.h
	test/ObjComponentTest.h
	test/ObjectTest.h
	test/OneToOneSpectraDetectorMapTest.h
	test/OrientedLatticeTest.h
	test/ParCompAssemblyTest.h
	test/ParComponentFactoryTest.h
	test/ParDetectorTest.h
	test/ParInstrumentTest.h
	test/ParObjCompAssemblyTest.h
	test/ParObjComponentTest.h
	test/ParameterMapTest.h
	test/ParametrizedComponentTest.h
	test/PlaneTest.h
	test/PointGroupTest.h
	test/PolygonEdgeTest.h
	test/QuadrilateralTest.h
	test/RectangularDetectorPixelTest.h
	test/RectangularDetectorTest.h
	test/ReducedCellTest.h
	test/ReferenceFrameTest.h
	test/ReflectionConditionTest.h
	test/RotCounterTest.h
	test/RulesBoolValueTest.h
	test/RulesCompGrpTest.h
	test/RulesCompObjTest.h
	test/RulesIntersectionTest.h
	test/RulesSurfPointTest.h
	test/RulesTest.h
	test/RulesUnionTest.h
	test/ScalarUtilsTest.h
	test/ShapeFactoryTest.h
	test/SphereTest.h
	test/SurfaceFactoryTest.h
	test/SurfaceTest.h
	test/TorusTest.h
	test/TrackTest.h
	test/TripleTest.h
	test/UnitCellTest.h
	test/Vertex2DListTest.h
	test/Vertex2DTest.h
    test/XMLlogFileTest.h
)

set ( GMOCK_TEST_FILES
    test/MDGeometryXMLBuilderTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Geometry SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Find 'local' dependencies
find_package ( OpenGL REQUIRED )
# FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
if ( NOT OPENGL_FOUND )
  message ( FATAL_ERROR "OpenGL was not found." )
endif ()

# ===================== Open Cascade ===================
if (NOT NO_OPENCASCADE)
  find_package ( OpenCascade REQUIRED )
  include_directories ( ${OPENCASCADE_INCLUDE_DIR} )
  set (SRC_FILES ${SRC_FILES} ${OPENCASCADE_SRC} )
else ()
  add_definitions ( -DNO_OPENCASCADE )
endif ()

# A few defines needed for OpenCascade on the Mac
if ( APPLE )
  add_definitions ( -DHAVE_IOSTREAM -DHAVE_LIMITS -DHAVE_IOMANIP )
endif ()

include_directories ( ${OPENGL_INCLUDE_DIR} )
include_directories ( ${GSL_INCLUDE_DIR} )


# Add a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidGeometry/PrecompiledHeader.h MantidGeometry src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( Geometry ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Geometry PROPERTIES OUTPUT_NAME MantidGeometry 
                                            COMPILE_DEFINITIONS IN_MANTID_GEOMETRY )
# Add to the 'Framework' group in VS
set_property ( TARGET Geometry PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Geometry ${MANTIDLIBS}  ${OPENGL_LIBRARIES} )

if (NOT NO_OPENCASCADE)
  target_link_libraries ( Geometry ${OPENCASCADE_LIBRARIES} )
endif ()

if ( CXXTEST_FOUND )
  # This is required to pick up ComponentCreationHelper, which is the only TestHelpers class that may be 
  # used by Geometry tests (as it does not depend on any higher package than geometry)
  include_directories( ../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( GeometryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( GeometryTest Geometry ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} Kernel)
  else ()
    cxxtest_add_test ( GeometryTest ${TEST_FILES} )
    target_link_libraries( GeometryTest Geometry Kernel)
  endif()
  
  add_dependencies ( FrameworkTests GeometryTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET GeometryTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Geometry  ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
