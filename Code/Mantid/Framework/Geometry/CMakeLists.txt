set ( SRC_FILES
	src/Crystal/UnitCell.cpp
	src/IDetector.cpp
	src/IInstrument.cpp
	src/IObjComponent.cpp
	src/Instrument/CompAssembly.cpp
	src/Instrument/Component.cpp
	src/Instrument/Detector.cpp
	src/Instrument/DetectorGroup.cpp
	src/Instrument/FitParameter.cpp
	src/Instrument/Instrument.cpp
	src/Instrument/NearestNeighbours.cpp
	src/Instrument/ObjCompAssembly.cpp
	src/Instrument/ObjComponent.cpp
	src/Instrument/ParComponentFactory.cpp
	src/Instrument/Parameter.cpp
	src/Instrument/ParameterMap.cpp
	src/Instrument/RectangularDetector.cpp
	src/Instrument/XMLlogfile.cpp
	src/InstrumentRayTracer.cpp
	src/MDGeometry/Coordinate.cpp
	src/MDGeometry/IMDDimension.cpp
	src/MDGeometry/MDBasisDimension.cpp
	src/MDGeometry/MDCell.cpp
	src/MDGeometry/MDDimension.cpp
	src/MDGeometry/MDDimensionRes.cpp
	src/MDGeometry/MDGeometry.cpp
	src/MDGeometry/MDGeometryBasis.cpp
	src/MDGeometry/MDGeometryDescription.cpp
	src/MDGeometry/MDHistoDimension.cpp
	src/MDGeometry/MDPoint.cpp
	src/Math/Acomp.cpp
	src/Math/Algebra.cpp
	src/Math/BnId.cpp
	src/Math/Matrix.cpp
	src/Math/MatrixBase.cpp
	src/Math/PolyBase.cpp
	src/Math/PolyFunction.cpp
	src/Math/PolyVar.cpp
	src/Math/PolyVarOne.cpp
	src/Math/RotCounter.cpp
	src/Math/Triple.cpp
	src/Math/mathSupport.cpp
	src/Objects/BoundingBox.cpp
	src/Objects/Material.cpp
	src/Objects/Object.cpp
	src/Objects/RuleItems.cpp
	src/Objects/Rules.cpp
	src/Objects/ShapeFactory.cpp
	src/Objects/Track.cpp
	src/Quat.cpp
	src/Rendering/BitmapGeometryHandler.cpp
	src/Rendering/CacheGeometryGenerator.cpp
	src/Rendering/CacheGeometryHandler.cpp
	src/Rendering/CacheGeometryRenderer.cpp
	src/Rendering/GeometryHandler.cpp
	src/Rendering/GluGeometryHandler.cpp
	src/Rendering/GluGeometryRenderer.cpp
	src/Rendering/OCGeometryGenerator.cpp
	src/Rendering/OCGeometryHandler.cpp
	src/Rendering/OCGeometryRenderer.cpp
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
	src/V3D.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/Instrument/CompAssembly.cpp
        src/Instrument/ObjCompAssembly.cpp 
        )
        
set ( INC_FILES
	inc/MantidGeometry/Crystal/UnitCell.h
	inc/MantidGeometry/DllExport.h
	inc/MantidGeometry/ICompAssembly.h
	inc/MantidGeometry/IComponent.h
	inc/MantidGeometry/IDetector.h
	inc/MantidGeometry/IInstrument.h
	inc/MantidGeometry/IObjComponent.h
	inc/MantidGeometry/Instrument/CompAssembly.h
	inc/MantidGeometry/Instrument/Component.h
	inc/MantidGeometry/Instrument/Detector.h
	inc/MantidGeometry/Instrument/DetectorGroup.h
	inc/MantidGeometry/Instrument/FitParameter.h
	inc/MantidGeometry/Instrument/Instrument.h
	inc/MantidGeometry/Instrument/NearestNeighbours.h
	inc/MantidGeometry/Instrument/ObjCompAssembly.h
	inc/MantidGeometry/Instrument/ObjComponent.h
	inc/MantidGeometry/Instrument/ParComponentFactory.h
	inc/MantidGeometry/Instrument/Parameter.h
	inc/MantidGeometry/Instrument/ParameterFactory.h
	inc/MantidGeometry/Instrument/ParameterMap.h
	inc/MantidGeometry/Instrument/RectangularDetector.h
	inc/MantidGeometry/Instrument/XMLlogfile.h
	inc/MantidGeometry/MDGeometry/Coordinate.h
	inc/MantidGeometry/MDGeometry/IMDDimension.h
	inc/MantidGeometry/MDGeometry/MDBasisDimension.h
	inc/MantidGeometry/MDGeometry/MDCell.h
	inc/MantidGeometry/MDGeometry/MDDimension.h
	inc/MantidGeometry/MDGeometry/MDDimensionRes.h
	inc/MantidGeometry/MDGeometry/MDGeometry.h
	inc/MantidGeometry/MDGeometry/MDGeometryBasis.h
	inc/MantidGeometry/MDGeometry/MDGeometryDescription.h
	inc/MantidGeometry/MDGeometry/MDHistoDimension.h
	inc/MantidGeometry/MDGeometry/MDPoint.h
	inc/MantidGeometry/MDGeometry/MDWorkspaceConstants.h
	inc/MantidGeometry/Math/Acomp.h
	inc/MantidGeometry/Math/Algebra.h
	inc/MantidGeometry/Math/BnId.h
	inc/MantidGeometry/Math/MapSupport.h
	inc/MantidGeometry/Math/Matrix.h
	inc/MantidGeometry/Math/MatrixBase.h
	inc/MantidGeometry/Math/PolyBase.h
	inc/MantidGeometry/Math/PolyFunction.h
	inc/MantidGeometry/Math/PolyVar.h
	inc/MantidGeometry/Math/RotCounter.h
	inc/MantidGeometry/Math/Triple.h
	inc/MantidGeometry/Math/mathSupport.h
	inc/MantidGeometry/Objects/BoundingBox.h
	inc/MantidGeometry/Objects/InstrumentRayTracer.h
	inc/MantidGeometry/Objects/Material.h
	inc/MantidGeometry/Objects/Object.h
	inc/MantidGeometry/Objects/Rules.h
	inc/MantidGeometry/Objects/ShapeFactory.h
	inc/MantidGeometry/Objects/Track.h
	inc/MantidGeometry/Quat.h
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
	inc/MantidGeometry/Tolerance.h
	inc/MantidGeometry/V3D.h
)

set ( TEST_FILES
	test/AcompTest.h
	test/AlgebraTest.h
	test/BnIdTest.h
	test/BoundingBoxTest.h
	test/CompAssemblyTest.h
	test/ComponentTest.h
	test/ConeTest.h
	test/CylinderTest.h
	test/DetectorGroupTest.h
	test/DetectorTest.h
	test/FitParameterTest.h
	test/GeneralTest.h
	test/InstrumentRayTracerTest.h
	test/InstrumentTest.h
	test/LineIntersectVisitTest.h
	test/LineTest.h
	test/MDCellTest.h
	test/MDDimensionTest.h
	test/MDGeometryBasisTest.h
	test/MDGeometryDescriptionTest.h
	test/MDGeometryTest.h
	test/MDHistoDimensionTest.h
	test/MDPointTest.h
	test/MapSupportTest.h
	test/MaterialTest.h
	test/MathSupportTest.h
	test/MatrixTest.h
	test/NearestNeighboursTest.h
	test/ObjCompAssemblyTest.h
	test/ObjComponentTest.h
	test/ObjectTest.h
	test/ParCompAssemblyTest.h
	test/ParDetectorTest.h
	test/ParInstrumentTest.h
	test/ParObjCompAssemblyTest.h
	test/ParObjComponentTest.h
	test/ParametrizedComponentTest.h
	test/PlaneTest.h
	test/PolyTest.h
	test/QuatTest.h
	test/RectangularDetectorTest.h
	test/RotCounterTest.h
	test/RulesBoolValueTest.h
	test/RulesCompGrpTest.h
	test/RulesCompObjTest.h
	test/RulesIntersectionTest.h
	test/RulesSurfPointTest.h
	test/RulesTest.h
	test/RulesUnionTest.h
	test/ShapeFactoryTest.h
	test/SphereTest.h
	test/SurfaceFactoryTest.h
	test/SurfaceTest.h
	test/TorusTest.h
	test/TrackTest.h
	test/TripleTest.h
	test/UnitCellTest.h
	test/V3DTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Geometry SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)

# Find 'local' dependencies
find_package ( OpenGL REQUIRED )
# FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
if ( NOT OPENGL_FOUND )
  message ( FATAL_ERROR "OpenGL was not found." )
endif ()

find_package ( OpenCascade REQUIRED )

include_directories ( ${OPENCASCADE_INCLUDE_DIR} )
include_directories ( ${OPENGL_INCLUDE_DIR} )
include_directories ( ${GSL_INCLUDE_DIR} )

# A few defines needed for OpenCascade on the Mac
if ( APPLE )
  add_definitions ( -DHAVE_IOSTREAM -DHAVE_LIMITS -DHAVE_IOMANIP )
endif ()

# Add the target for this directory
add_library ( Geometry ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Geometry PROPERTIES OUTPUT_NAME MantidGeometry 
                                            COMPILE_DEFINITIONS IN_MANTID_GEOMETRY )
# Add to the 'Framework' group in VS
set_property ( TARGET Geometry PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Geometry ${MANTIDLIBS} ${OPENCASCADE_LIBRARIES} ${OPENGL_LIBRARIES} ${GSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  cxxtest_add_test ( GeometryTest ${TEST_FILES} )
  include_directories( ../TestHelpers/inc ../DataObjects/inc ../API/inc )
  target_link_libraries( GeometryTest Geometry TestHelpers )
  add_dependencies ( FrameworkTests GeometryTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET GeometryTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Geometry DESTINATION ${LIB_DIR} )
