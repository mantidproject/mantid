add_subdirectory(main)
add_subdirectory(sans)
add_subdirectory(refl)

add_custom_target(CompileUIDrill
                  DEPENDS CompileUIDrillMain
                          CompileUIDrillSans
                          CompileUIDrillRefl)
