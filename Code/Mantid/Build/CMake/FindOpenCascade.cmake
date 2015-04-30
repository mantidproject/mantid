# - try to find OpenCascade Library and include files
# OPENCASCADE_INCLUDE_DIR where to find Standard.hxx, etc.
# OPENCASCADE_LIBRARIES libraries to link against
# OPENCASCADE_FOUND If false, do not try to use OPENCASCADE

find_path ( OPENCASCADE_INCLUDE_DIR Standard.hxx PATHS
                /usr/include/opencascade
                /usr/include/oce
                /usr/local/include/opencascade
                /usr/local/include/oce
                /usr/local/inc
                /opt/OpenCASCADE/inc
                ${CMAKE_INCLUDE_PATH}/OpenCascade
                $ENV{CASROOT}/inc
)

if ( WIN32 )
  add_definitions ( -DWNT )
endif ( WIN32 )

find_path ( OPENCASCADE_LIBRARY_DIR libTKernel.so PATHS
                /opt/OpenCASCADE/lib64
                $ENV{CASROOT}/lib64
                /opt/OpenCASCADE/lib
                $ENV{CASROOT}/lib
                /opt/OpenCASCADE/lib32
                $ENV{CASROOT}/lib32
)

find_library ( OPENCASCADE_LIB_TKERNEL
                 NAMES TKernel
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKBO
                 NAMES TKBO
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKPRIM
                 NAMES TKPrim
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKMESH
                 NAMES TKMesh
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKBREP
                 NAMES TKBRep
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKTOPALGO
                 NAMES TKTopAlgo
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKMATH
                 NAMES TKMath
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)
find_library ( OPENCASCADE_LIB_TKG2D
                 NAMES TKG2d
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)

find_library ( OPENCASCADE_LIB_TKG3D
                 NAMES TKG3d
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)

find_library ( OPENCASCADE_LIB_TKGEOMBASE
                 NAMES TKGeomBase
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)

find_library ( OPENCASCADE_LIB_TKGEOMALGO
                 NAMES TKGeomAlgo
                 PATHS ${OPENCASCADE_LIBRARY_DIR}
)

set ( OPENCASCADE_LIBRARIES
        ${OPENCASCADE_LIB_TKERNEL}
        ${OPENCASCADE_LIB_TKBO}
        ${OPENCASCADE_LIB_TKPRIM}
        ${OPENCASCADE_LIB_TKMESH}
        ${OPENCASCADE_LIB_TKBREP}
        ${OPENCASCADE_LIB_TKTOPALGO}
        ${OPENCASCADE_LIB_TKMATH}
        ${OPENCASCADE_LIB_TKG2D}
        ${OPENCASCADE_LIB_TKG3D}
        ${OPENCASCADE_LIB_TKGEOMBASE}
        ${OPENCASCADE_LIB_TKGEOMALGO}
)

# handle the QUIETLY and REQUIRED arguments and set OPENCASCADE_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( OpenCascade DEFAULT_MSG OPENCASCADE_LIBRARIES OPENCASCADE_INCLUDE_DIR )

mark_as_advanced ( OPENCASCADE_INCLUDE_DIR OPENCASCADE_LIBRARY_DIR
                   OPENCASCADE_LIB_TKERNEL OPENCASCADE_LIB_TKBO 
                   OPENCASCADE_LIB_TKPRIM OPENCASCADE_LIB_TKMESH
                   OPENCASCADE_LIB_TKBREP OPENCASCADE_LIB_TKTOPALGO 
                   OPENCASCADE_LIB_TKMATH OPENCASCADE_LIB_TKG2D
                   OPENCASCADE_LIB_TKG3D OPENCASCADE_LIB_TKGEOMBASE
                   OPENCASCADE_LIB_TKGEOMALGO
)
