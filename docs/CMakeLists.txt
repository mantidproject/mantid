# Sphinx documentation

# Build options
option(DOCS_DOTDIAGRAMS "If enabled include workflow diagrams generated with dot" OFF)
option(DOCS_SCREENSHOTS "If enabled include automatically generated screenshots of interfaces" OFF)
set(DOCS_MATH_EXT "sphinx.ext.mathjax" CACHE STRING "Sphinx extension to be used for rendering math equations. Default uses mathjax for speed")
set_property(CACHE DOCS_MATH_EXT PROPERTY STRINGS "sphinx.ext.imgmath" "sphinx.ext.mathjax")
option(DOCS_PLOTDIRECTIVE "If enabled include plots generated with the plot directive. " OFF)

# Build layout
set(SPHINX_CONF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(SPHINX_BUILD_DIR ${DOCS_BUILDDIR})
set(DOCTREES_DIR ${DOCS_BUILDDIR}/doctrees)
if(DOCS_DOTDIAGRAMS)
  if(NOT DOXYGEN_DOT_EXECUTABLE)
    message(FATAL_ERROR "DOCS_DOTDIAGRAMS enabled but dot executable cannot be found")
  endif()
  set(DOT_EXECUTABLE ${DOXYGEN_DOT_EXECUTABLE})
  set(DIAGRAMS_DIR ${SPHINX_BUILD_DIR}/diagrams)
else()
  set(DIAGRAMS_DIR "")
endif()
if(DOCS_SCREENSHOTS)
  set(SCREENSHOTS_DIR ${SPHINX_BUILD_DIR}/screenshots)
else()
  set(SCREENSHOTS_DIR "")
endif()
if(DOCS_PLOTDIRECTIVE)
  set(ENABLE_PLOTDIRECTIVE 1)
else()
  set(ENABLE_PLOTDIRECTIVE "")
endif()

# Add a sphinx build target to build documentation
#   builder - Name of the sphinx builder: html, qthelp etc
#   target_name - Optional target name. Default=docs-${builder}
function (add_sphinx_build_target builder)
  set(runner ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantidpython)
  set(runner_args "--classic")
  if(ARGC GREATER 1)
    set(target_name ${ARGV1})
  else()
    set(target_name docs-${builder})
  endif()
  set(output_dir ${SPHINX_BUILD_DIR}/${builder})
  if(EXISTS ${SPHINX_CONF_DIR}/conf-${builder}.py)
    set(conf_builder ${SPHINX_CONF_DIR}/conf-${builder}.py)
  endif()
  add_custom_target(${target_name}
      COMMAND
        ${CMAKE_COMMAND} -E env
             SCREENSHOTS_DIR=${SCREENSHOTS_DIR}
             DIAGRAMS_DIR=${DIAGRAMS_DIR}
             DOT_EXECUTABLE=${DOT_EXECUTABLE}
             MATH_EXT=${DOCS_MATH_EXT}
             ENABLE_PLOTDIRECTIVE=${ENABLE_PLOTDIRECTIVE}
             ${runner} ${runner_args} -m sphinx --no-color -b ${builder} -d ${DOCTREES_DIR} ${SPHINX_CONF_DIR} ${output_dir}
      DEPENDS Framework
              mantidqt
              ${SPHINX_CONF_DIR}/conf.py
              ${conf_builder}
      COMMENT "Building ${builder} documentation"
  )
  # Group within VS and exclude from whole build
  set_target_properties(${target_name}
      PROPERTIES FOLDER "Documentation"
      EXCLUDE_FROM_DEFAULT_BUILD 1
      EXCLUDE_FROM_ALL 1
  )
endfunction()

# Documentation types
add_sphinx_build_target(html)
add_sphinx_build_target(doctest)

# qthelp
if(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
  add_sphinx_build_target(qthelp docs-qtassistant)
  add_custom_target(docs-qthelp
                    COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} qthelp/MantidProject.qhcp
                    COMMENT "Compiling Qt help documentation")
  add_dependencies(docs-qthelp docs-qtassistant)
else()
  message(WARNING "Did not find qcollectiongenerator - cannot create docs-qthelp target.")
endif()

# Installation settings
option(PACKAGE_DOCS "If true the qthelp documentation is bundled with the package" OFF)
if(PACKAGE_DOCS)
  set(HTML_DOCS_DEST share/doc)
  foreach(_bundle ${BUNDLES})
    if(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
      # must "make docs-qthelp" before "make package" otherwise there will be a
      # build failure
      install(FILES ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qhc
                    ${SPHINX_BUILD_DIR}/qthelp/MantidProject.qch
              DESTINATION ${_bundle}${HTML_DOCS_DEST})
      install(DIRECTORY ${SPHINX_BUILD_DIR}/qthelp/
              DESTINATION ${_bundle}${HTML_DOCS_DEST}
              FILES_MATCHING
              PATTERN "*.html")
      install(DIRECTORY ${SPHINX_BUILD_DIR}/qthelp/_images/
                        ${SPHINX_BUILD_DIR}/qthelp/_static/
              DESTINATION ${_bundle}${HTML_DOCS_DEST})
    else(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
      install(DIRECTORY ${SPHINX_BUILD_DIR}/html/
              DESTINATION ${_bundle}${HTML_DOCS_DEST})
    endif(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
  endforeach()
endif(PACKAGE_DOCS)
