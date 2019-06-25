# Pinched the bulk of this file from:
# http://bugreports.qt.nokia.com/browse/QTSOLBUG-80

project(QtPropertyBrowser)

# We check for Qt in the directory above this, but having this here allows
# building as a stand-alone project
if(NOT QT_FOUND)
  find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
endif()
include(UseQt4)

# Add the sources
set(QTPROPERTYBROWSER_SRCS
    src/qtpropertybrowser.cpp
    src/qtpropertymanager.cpp
    src/qteditorfactory.cpp
    src/qtvariantproperty.cpp
    src/qttreepropertybrowser.cpp
    src/qtbuttonpropertybrowser.cpp
    src/qtgroupboxpropertybrowser.cpp
    src/qtpropertybrowserutils.cpp
    src/DoubleDialogEditor.cpp
    src/DoubleEditorFactory.cpp
    src/FilenameDialogEditor.cpp
    src/FormulaDialogEditor.cpp
    src/ParameterPropertyManager.cpp
    src/StringDialogEditor.cpp
    src/StringEditorFactory.cpp)

# moc'd files will end up in build directory, so add to include path
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Need to use a string of calls to the qt4_generate_moc macro instead of
# qt4_wrap_cpp because the latter produces files with a cxx suffix when
# QtPropertyBrowser requires cpp

qt4_generate_moc(
  src/qtpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowser.cpp)
qt4_generate_moc(
  src/qtpropertymanager.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertymanager.cpp)
qt4_generate_moc(
  src/qteditorfactory.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qteditorfactory.cpp)
qt4_generate_moc(
  src/qtvariantproperty.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtvariantproperty.cpp)
qt4_generate_moc(
  src/qttreepropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qttreepropertybrowser.cpp)
qt4_generate_moc(
  src/qtbuttonpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtbuttonpropertybrowser.cpp)
qt4_generate_moc(
  src/qtgroupboxpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtgroupboxpropertybrowser.cpp)
qt4_generate_moc(
  src/qtpropertymanager.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qtpropertymanager.moc)
qt4_generate_moc(
  src/qteditorfactory.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qteditorfactory.moc)
qt4_generate_moc(
  src/qttreepropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qttreepropertybrowser.moc)

set(QTPROPERTYBROWSER_HEADERS_ONLY_MOC
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertymanager.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qteditorfactory.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtvariantproperty.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qttreepropertybrowser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtbuttonpropertybrowser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtgroupboxpropertybrowser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/qtpropertymanager.moc
    ${CMAKE_CURRENT_BINARY_DIR}/qteditorfactory.moc
    ${CMAKE_CURRENT_BINARY_DIR}/qttreepropertybrowser.moc)

set_source_files_properties(${QTPROPERTYBROWSER_HEADERS_ONLY_MOC}
                            PROPERTIES
                            HEADER_FILE_ONLY
                            true)

qt4_generate_moc(
  src/qtpropertybrowserutils_p.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowserutils_p.cpp)

set(QTPROPERTYBROWSER_HEADERS_MOC
    ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowserutils_p.cpp)

qt4_wrap_cpp(EXTRA_MOCS
             src/ButtonEditorFactory.h
             src/DoubleEditorFactory.h
             src/DoubleDialogEditor.h
             src/FormulaDialogEditor.h
             src/FilenameDialogEditor.h
             src/ParameterPropertyManager.h
             src/StringDialogEditor.h
             src/StringEditorFactory.h)

set(QTPROPERTYBROWSER_MOC
    ${QTPROPERTYBROWSER_HEADERS_MOC}
    ${QTPROPERTYBROWSER_HEADERS_ONLY_MOC}
    ${EXTRA_MOCS})

qt4_add_resources(
  QTPROPERTYBROWSER_RESOURCES
  src/qtpropertybrowser.qrc)

set(SRCS
    ${QTPROPERTYBROWSER_MOC}
    ${QTPROPERTYBROWSER_SRCS}
    ${QTPROPERTYBROWSER_RESOURCES})

add_library(${PROJECT_NAME} SHARED ${SRCS})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(${PROJECT_NAME}
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(${PROJECT_NAME}
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

target_link_libraries(${PROJECT_NAME}
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${QT_LIBRARIES})

if(WIN32)
  add_definitions(-DQT_QTPROPERTYBROWSER_EXPORT)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-DQT_QTPROPERTYBROWSER_EXPORT)
endif()

# This is, for the most part, not our code so disable an Intel compiler warning
# that crops up a lot in this package
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
  add_definitions(-wd1125)
  # Also make sure the equivalent gcc warning is turned off
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  add_definitions(-Wno-overloaded-virtual)
  # And one other one as well that GCC 4.6 complains about
  add_definitions(-Wno-unused-but-set-variable)
endif()

# Installation settings
install(TARGETS ${PROJECT_NAME} DESTINATION ${LIB_DIR})
