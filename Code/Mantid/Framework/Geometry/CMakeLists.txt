set ( SRC_FILES
	src/ComponentParser.cpp
	src/Crystal/BraggScatterer.cpp
	src/Crystal/BraggScattererFactory.cpp
	src/Crystal/BraggScattererInCrystalStructure.cpp
	src/Crystal/CenteringGroup.cpp
	src/Crystal/CompositeBraggScatterer.cpp
	src/Crystal/ConventionalCell.cpp
	src/Crystal/CrystalStructure.cpp
	src/Crystal/CyclicGroup.cpp
	src/Crystal/Group.cpp
	src/Crystal/IndexingUtils.cpp
	src/Crystal/IsotropicAtomBraggScatterer.cpp
	src/Crystal/NiggliCell.cpp
	src/Crystal/OrientedLattice.cpp
	src/Crystal/PointGroup.cpp
	src/Crystal/PointGroupFactory.cpp
	src/Crystal/ProductOfCyclicGroups.cpp
	src/Crystal/ReducedCell.cpp
	src/Crystal/ReflectionCondition.cpp
	src/Crystal/ScalarUtils.cpp
	src/Crystal/SpaceGroup.cpp
	src/Crystal/SpaceGroupFactory.cpp
	src/Crystal/SymmetryOperation.cpp
	src/Crystal/SymmetryOperationFactory.cpp
	src/Crystal/SymmetryOperationSymbolParser.cpp
	src/Crystal/UnitCell.cpp
	src/Crystal/V3R.cpp
	src/IObjComponent.cpp
	src/Instrument.cpp
	src/Instrument/CompAssembly.cpp
	src/Instrument/Component.cpp
	src/Instrument/ComponentHelper.cpp
	src/Instrument/Detector.cpp
	src/Instrument/DetectorGroup.cpp
	src/Instrument/FitParameter.cpp
	src/Instrument/Goniometer.cpp
	src/Instrument/IDFObject.cpp
	src/Instrument/InstrumentDefinitionParser.cpp
	src/Instrument/NearestNeighbours.cpp
	src/Instrument/NearestNeighboursFactory.cpp
	src/Instrument/ObjCompAssembly.cpp
	src/Instrument/ObjComponent.cpp
	src/Instrument/ParComponentFactory.cpp
	src/Instrument/Parameter.cpp
	src/Instrument/ParameterMap.cpp
	src/Instrument/RectangularDetector.cpp
	src/Instrument/RectangularDetectorPixel.cpp
	src/Instrument/ReferenceFrame.cpp
	src/Instrument/XMLInstrumentParameter.cpp
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
	inc/MantidGeometry/Crystal/BraggScatterer.h
	inc/MantidGeometry/Crystal/BraggScattererFactory.h
	inc/MantidGeometry/Crystal/BraggScattererInCrystalStructure.h
	inc/MantidGeometry/Crystal/CenteringGroup.h
	inc/MantidGeometry/Crystal/CompositeBraggScatterer.h
	inc/MantidGeometry/Crystal/ConventionalCell.h
	inc/MantidGeometry/Crystal/CrystalStructure.h
	inc/MantidGeometry/Crystal/CyclicGroup.h
	inc/MantidGeometry/Crystal/Group.h
	inc/MantidGeometry/Crystal/IndexingUtils.h
	inc/MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h
	inc/MantidGeometry/Crystal/NiggliCell.h
	inc/MantidGeometry/Crystal/OrientedLattice.h
	inc/MantidGeometry/Crystal/PointGroup.h
	inc/MantidGeometry/Crystal/PointGroupFactory.h
	inc/MantidGeometry/Crystal/ProductOfCyclicGroups.h
	inc/MantidGeometry/Crystal/Projection.h
	inc/MantidGeometry/Crystal/ReducedCell.h
	inc/MantidGeometry/Crystal/ReflectionCondition.h
	inc/MantidGeometry/Crystal/ScalarUtils.h
	inc/MantidGeometry/Crystal/SpaceGroup.h
	inc/MantidGeometry/Crystal/SpaceGroupFactory.h
	inc/MantidGeometry/Crystal/SymmetryOperation.h
	inc/MantidGeometry/Crystal/SymmetryOperationFactory.h
	inc/MantidGeometry/Crystal/SymmetryOperationSymbolParser.h
	inc/MantidGeometry/Crystal/UnitCell.h
	inc/MantidGeometry/Crystal/V3R.h
	inc/MantidGeometry/DllConfig.h
	inc/MantidGeometry/ICompAssembly.h
	inc/MantidGeometry/IComponent.h
	inc/MantidGeometry/IDetector.h
	inc/MantidGeometry/IObjComponent.h
	inc/MantidGeometry/Instrument.h
	inc/MantidGeometry/Instrument/CompAssembly.h
	inc/MantidGeometry/Instrument/Component.h
	inc/MantidGeometry/Instrument/ComponentHelper.h
	inc/MantidGeometry/Instrument/Detector.h
	inc/MantidGeometry/Instrument/DetectorGroup.h
	inc/MantidGeometry/Instrument/FitParameter.h
	inc/MantidGeometry/Instrument/Goniometer.h
	inc/MantidGeometry/Instrument/IDFObject.h
	inc/MantidGeometry/Instrument/INearestNeighbours.h
	inc/MantidGeometry/Instrument/INearestNeighboursFactory.h
	inc/MantidGeometry/Instrument/InstrumentDefinitionParser.h
	inc/MantidGeometry/Instrument/NearestNeighbours.h
	inc/MantidGeometry/Instrument/NearestNeighboursFactory.h
	inc/MantidGeometry/Instrument/ObjCompAssembly.h
	inc/MantidGeometry/Instrument/ObjComponent.h
	inc/MantidGeometry/Instrument/ParComponentFactory.h
	inc/MantidGeometry/Instrument/Parameter.h
	inc/MantidGeometry/Instrument/ParameterFactory.h
	inc/MantidGeometry/Instrument/ParameterMap.h
	inc/MantidGeometry/Instrument/RectangularDetector.h
	inc/MantidGeometry/Instrument/RectangularDetectorPixel.h
	inc/MantidGeometry/Instrument/ReferenceFrame.h
	inc/MantidGeometry/Instrument/XMLInstrumentParameter.h
	inc/MantidGeometry/MDGeometry/CompositeImplicitFunction.h
	inc/MantidGeometry/MDGeometry/IMDDimension.h
	inc/MantidGeometry/MDGeometry/IMDDimensionFactory.h
	inc/MantidGeometry/MDGeometry/MDBoxImplicitFunction.h
	inc/MantidGeometry/MDGeometry/MDDimensionExtents.h
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
	inc/MantidGeometry/Rendering/OpenGL_Headers.h
	inc/MantidGeometry/Rendering/vtkGeometryCacheReader.h
	inc/MantidGeometry/Rendering/vtkGeometryCacheWriter.h
	inc/MantidGeometry/Surfaces/BaseVisit.h
)

set ( TEST_FILES
	AcompTest.h
	AlgebraTest.h
	BnIdTest.h
	BoundingBoxTest.h
	BraggScattererFactoryTest.h
	BraggScattererInCrystalStructureTest.h
	BraggScattererTest.h
	CenteringGroupTest.h
	CompAssemblyTest.h
	ComponentHelperTest.h
	ComponentParserTest.h
	ComponentTest.h
	CompositeBraggScattererTest.h
	CompositeImplicitFunctionTest.h
	ConeTest.h
	ConventionalCellTest.h
	ConvexPolygonTest.h
	CrystalStructureTest.h
	CyclicGroupTest.h
	CylinderTest.h
	DetectorGroupTest.h
	DetectorTest.h
	FitParameterTest.h
	GeneralTest.h
	GoniometerTest.h
	GroupTest.h
	IDFObjectTest.h
	IMDDimensionFactoryTest.h
	IMDDimensionTest.h
	IndexingUtilsTest.h
	InstrumentDefinitionParserTest.h
	InstrumentRayTracerTest.h
	InstrumentTest.h
	IsotropicAtomBraggScattererTest.h
	LaszloIntersectionTest.h
	LineIntersectVisitTest.h
	LineTest.h
	MDBoxImplicitFunctionTest.h
	MDGeometryXMLParserTest.h
	MDHistoDimensionBuilderTest.h
	MDHistoDimensionTest.h
	MDImplicitFunctionTest.h
	MDPlaneImplicitFunctionTest.h
	MDPlaneTest.h
	MapSupportTest.h
	MathSupportTest.h
	NearestNeighboursFactoryTest.h
	NearestNeighboursTest.h
	NiggliCellTest.h
	NullImplicitFunctionTest.h
	ObjCompAssemblyTest.h
	ObjComponentTest.h
	ObjectTest.h
	OrientedLatticeTest.h
	ParCompAssemblyTest.h
	ParComponentFactoryTest.h
	ParDetectorTest.h
	ParInstrumentTest.h
	ParObjCompAssemblyTest.h
	ParObjComponentTest.h
	ParameterMapTest.h
	ParametrizedComponentTest.h
	PlaneTest.h
	PointGroupFactoryTest.h
	PointGroupTest.h
	PolygonEdgeTest.h
	ProductOfCyclicGroupsTest.h
	QuadrilateralTest.h
	RectangularDetectorPixelTest.h
	RectangularDetectorTest.h
	ReducedCellTest.h
	ReferenceFrameTest.h
	ReflectionConditionTest.h
	RotCounterTest.h
	RulesBoolValueTest.h
	RulesCompGrpTest.h
	RulesCompObjTest.h
	RulesIntersectionTest.h
	RulesSurfPointTest.h
	RulesTest.h
	RulesUnionTest.h
	ScalarUtilsTest.h
	ShapeFactoryTest.h
	SpaceGroupFactoryTest.h
	SpaceGroupTest.h
	SphereTest.h
	SurfaceFactoryTest.h
	SurfaceTest.h
	SymmetryOperationFactoryTest.h
	SymmetryOperationSymbolParserTest.h
	SymmetryOperationTest.h
	TorusTest.h
	TrackTest.h
	TripleTest.h
	UnitCellTest.h
	V3RTest.h
	Vertex2DListTest.h
	Vertex2DTest.h
	XMLInstrumentParameterTest.h
)

set ( GMOCK_TEST_FILES
  MDGeometryXMLBuilderTest.h
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
  include_directories ( system ${OPENCASCADE_INCLUDE_DIR} )
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
enable_precompiled_headers( inc/MantidGeometry/PrecompiledHeader.h SRC_FILES )
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

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Geometry  ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
