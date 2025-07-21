# Sphinx documentation

# Build options
option(DOCS_DOTDIAGRAMS "If enabled include workflow diagrams generated with dot" OFF)
option(DOCS_SCREENSHOTS "If enabled include automatically generated screenshots of interfaces" OFF)
set(DOCS_MATH_EXT
    "sphinx.ext.imgmath"
    CACHE STRING "Sphinx extension to be used for rendering math equations in qtassistant. mathjax is used in html."
)
set_property(CACHE DOCS_MATH_EXT PROPERTY STRINGS "sphinx.ext.imgmath" "sphinx.ext.mathjax")
option(DOCS_PLOTDIRECTIVE "If enabled include plots generated with the plot directive. " OFF)
option(SPHINX_WARNINGS_AS_ERRORS "non-zero exit if there are sphinx warnings" ON)
option(SPHINX_FRESH_ENV "Don't use a saved environment, but rebuild it completely" ON)

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
if(SPHINX_WARNINGS_AS_ERRORS)
  set(SPHINX_WARNINGS_AS_ERRORS_FLAG "-W")
else()
  set(SPHINX_WARNINGS_AS_ERRORS_FLAG "")
endif()
if(SPHINX_FRESH_ENV)
  set(SPHINX_FRESH_ENV_FLAG "-E")
else()
  set(SPHINX_FRESH_ENV_FLAG "")
endif()

# Add a sphinx build target to build documentation builder - Name of the sphinx builder: html, qthelp etc target_name -
# Optional target name. Default=docs-${builder}
function(add_sphinx_build_target builder math_renderer)

  if(ARGC GREATER 2)
    set(target_name ${ARGV2})
  else()
    set(target_name docs-${builder})
  endif()

  set(output_dir ${SPHINX_BUILD_DIR}/${builder})
  if(EXISTS ${SPHINX_CONF_DIR}/conf-${builder}.py)
    set(conf_builder ${SPHINX_CONF_DIR}/conf-${builder}.py)
  endif()

  # -W turns warnings into errors, but there is an issue with getting the numpy intersphinx.
  set(sphinx_options
      ${SPHINX_NOCOLOR}
      ${SPHINX_KEEPGOING}
      ${SPHINX_WARNINGS_AS_ERRORS_FLAG}
      ${SPHINX_FRESH_ENV_FLAG}
      -b
      ${builder}
      -d
      ${DOCTREES_DIR}
  )
  # add a tag to differentiate between html/qthelp in conf
  if(ARGC GREATER 3)
    set(sphinx_options ${sphinx_options} -t ${ARGV3})
  endif()

  set(target_dependencies)

  if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
    set(target_dependencies ${target_dependencies} Framework)
  endif()

  if(MANTID_QT_LIB STREQUAL "BUILD")
    set(target_dependencies ${target_dependencies} mantidqt)
  endif()

  add_custom_target(
    ${target_name}
    COMMAND
      ${CMAKE_COMMAND} -E env SCREENSHOTS_DIR=${SCREENSHOTS_DIR} DIAGRAMS_DIR=${DIAGRAMS_DIR}
      DOT_EXECUTABLE=${DOT_EXECUTABLE} MATH_EXT=${math_renderer} ENABLE_PLOTDIRECTIVE=${ENABLE_PLOTDIRECTIVE}
      PYTHONPATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} ${Python_EXECUTABLE} -m sphinx.cmd.build
      ${sphinx_options} ${SPHINX_CONF_DIR} ${output_dir}
    DEPENDS ${target_dependencies} ${SPHINX_CONF_DIR}/conf.py ${conf_builder}
    COMMENT "Building ${builder} documentation using ${math_renderer}"
  )
  # Group within VS and exclude from whole build
  set_target_properties(
    ${target_name}
    PROPERTIES FOLDER "Documentation"
               EXCLUDE_FROM_DEFAULT_BUILD 1
               EXCLUDE_FROM_ALL 1
  )
endfunction()

# Documentation types
add_sphinx_build_target(html "sphinx.ext.mathjax")
add_sphinx_build_target(doctest "sphinx.ext.mathjax")
add_dependencies(docs-doctest DocTestData)

# add target for checking external links
add_sphinx_build_target(linkcheck ${DOCS_MATH_EXT})

# The following lines related to qthelp should remain commented or removed add_sphinx_build_target(qthelp
# ${DOCS_MATH_EXT} qthelp) add_dependencies(docs-qthelp ${QCH_FILES})

# Installation settings
option(PACKAGE_DOCS "If true the HTML documentation is bundled with the package" OFF)
if(PACKAGE_DOCS)
  set(HTML_DOCS_DEST share/doc/html)
  foreach(_bundle ${BUNDLES})
    # Always install HTML documentation if PACKAGE_DOCS is true This comes from the 'docs-html' sphinx target's output
    # directory
    message(STATUS "Installing HTML documentation to ${_bundle}${HTML_DOCS_DEST} from ${SPHINX_BUILD_DIR}/html/")
    install(
      DIRECTORY ${SPHINX_BUILD_DIR}/html/
      DESTINATION ${_bundle}${HTML_DOCS_DEST}
      COMPONENT Documentation
    )
  endforeach()
endif(PACKAGE_DOCS)
