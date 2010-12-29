# - try to find OpenCascade Library and include files
# OPENCASCADE_INCLUDE_DIR where to find Standard.hxx, etc.
# OPENCASCADE_LIBRARIES libraries to link against
# OPENCASCADE_FOUND If false, do not try to use OPENCASCADE

find_path ( OPENCASCADE_INCLUDE_DIR Standard.hxx PATHS
                /usr/include/opencascade
                /usr/local/include/opencascade
                /usr/local/inc
                /opt/OpenCASCADE/inc
                ${CMAKE_INCLUDE_PATH}/OpenCascade
                $ENV{CASROOT}/inc
)

if ( WIN32 )
  add_definitions ( -DWNT )
endif ( WIN32 )

set ( OC_REDHAT_RPM /opt/OpenCASCADE/lib64 )
find_library ( OPENCASCADE_LIB_TKERNEL
                 NAMES TKernel
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKBO
                 NAMES TKBO
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKPRIM
                 NAMES TKPrim
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKMESH
                 NAMES TKMesh
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKBREP
                 NAMES TKBRep
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKTOPALGO
                 NAMES TKTopAlgo
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKMATH
                 NAMES TKMath
                 PATHS ${OC_REDHAT_RPM}         
)
find_library ( OPENCASCADE_LIB_TKG2D
                 NAMES TKG2d
                 PATHS ${OC_REDHAT_RPM}         
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
)

# handle the QUIETLY and REQUIRED arguments and set OPENCASCADE_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( OpenCascade DEFAULT_MSG OPENCASCADE_LIBRARIES OPENCASCADE_INCLUDE_DIR )

mark_as_advanced ( ${OPENCASCADE_LIB_TKERNEL} ${OPENCASCADE_LIB_TKBO} ${OPENCASCADE_LIB_TKPRIM} ${OPENCASCADE_LIB_TKMESH}
                   ${OPENCASCADE_LIB_TKBREP} ${OPENCASCADE_LIB_TKTOPALGO} ${OPENCASCADE_LIB_TKMATH} ${OPENCASCADE_LIB_TKG2D}
)
