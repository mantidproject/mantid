# Create a mantid python conda constructor environment

set(PATH_TO_CONSTRUCT ${PROJECT_SOURCE_DIR}/installers/constructor)
if(WIN32)
  set(OUTPUT_ENV "${PATH_TO_CONSTRUCT}/MantidPython-0.1.0-Windows-x86_64.exe")
else()
  set(OUTPUT_ENV "${PATH_TO_CONSTRUCT}/MantidPython-0.1.0-MacOSX-x86_64.sh")
endif()

add_custom_command(
  OUTPUT ${OUTPUT_ENV}
  COMMAND constructor ${PATH_TO_CONSTRUCT} --output-dir=${PATH_TO_CONSTRUCT}
  COMMENT "Creating conda construct python package"
)

add_custom_target(ConstructorEnv ALL DEPENDS ${OUTPUT_ENV})

if(APPLE)
  install(CODE "set(WORKBENCH_APP \"${WORKBENCH_APP}\")" COMPONENT Runtime)
  install(
    FILES ${OUTPUT_ENV}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${WORKBENCH_APP}/Contents/Resources/env_installer
    COMPONENT Runtime
  )
else()
  install(
    FILES ${OUTPUT_ENV}
    DESTINATION resources/env_installer
    COMPONENT Runtime
  )
endif()
