#=============================================================================
# Taken from CMake file UseQt4.cmake and modified to use the SYSTEM option
# for the include_directories so that the headers are treated as system 
# headers and will not throw warnings
#=============================================================================

# - Use Module for QT4
# Sets up C and C++ to use Qt 4.  It is assumed that FindQt.cmake
# has already been loaded.  See FindQt.cmake for information on
# how to load Qt 4 into your CMake project.

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

ADD_DEFINITIONS(${QT_DEFINITIONS})
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.9)
  SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:QT_DEBUG>)
  SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>)
ELSE()
  SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG QT_DEBUG)
  SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG)
ENDIF()
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG)
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG)

INCLUDE_DIRECTORIES(SYSTEM ${QT_INCLUDE_DIR})
IF(Q_WS_MAC AND QT_USE_FRAMEWORKS)
  INCLUDE_DIRECTORIES(SYSTEM ${QT_QTCORE_LIBRARY})
ENDIF(Q_WS_MAC AND QT_USE_FRAMEWORKS)

SET(QT_LIBRARIES "")

IF (QT_USE_QTMAIN)
  IF (WIN32)
    SET(QT_LIBRARIES ${QT_LIBRARIES} ${QT_QTMAIN_LIBRARY})
  ENDIF (WIN32)
ENDIF (QT_USE_QTMAIN)

IF(QT_DONT_USE_QTGUI)
  SET(QT_USE_QTGUI 0)
ELSE(QT_DONT_USE_QTGUI)
  SET(QT_USE_QTGUI 1)
ENDIF(QT_DONT_USE_QTGUI)

IF(QT_DONT_USE_QTCORE)
  SET(QT_USE_QTCORE 0)
ELSE(QT_DONT_USE_QTCORE)
  SET(QT_USE_QTCORE 1)
ENDIF(QT_DONT_USE_QTCORE)

IF (QT_USE_QT3SUPPORT)
  ADD_DEFINITIONS(-DQT3_SUPPORT)
ENDIF (QT_USE_QT3SUPPORT)

# list dependent modules, so dependent libraries are added
SET(QT_QT3SUPPORT_MODULE_DEPENDS QTGUI QTSQL QTXML QTNETWORK QTCORE)
SET(QT_QTSVG_MODULE_DEPENDS QTGUI QTXML QTCORE)
SET(QT_QTUITOOLS_MODULE_DEPENDS QTGUI QTXML QTCORE)
SET(QT_QTHELP_MODULE_DEPENDS QTGUI QTSQL QTXML QTNETWORK QTCORE)
IF(QT_QTDBUS_FOUND)
  SET(QT_PHONON_MODULE_DEPENDS QTGUI QTDBUS QTCORE)
ELSE(QT_QTDBUS_FOUND)
  SET(QT_PHONON_MODULE_DEPENDS QTGUI QTCORE)
ENDIF(QT_QTDBUS_FOUND)
SET(QT_QTDBUS_MODULE_DEPENDS QTXML QTCORE)
SET(QT_QTXMLPATTERNS_MODULE_DEPENDS QTNETWORK QTCORE)
SET(QT_QAXCONTAINER_MODULE_DEPENDS QTGUI QTCORE)
SET(QT_QAXSERVER_MODULE_DEPENDS QTGUI QTCORE)
SET(QT_QTSCRIPTTOOLS_MODULE_DEPENDS QTGUI QTCORE)

# Qt modules  (in order of dependence)
FOREACH(module QT3SUPPORT QTOPENGL QTDESIGNER QTMOTIF QTNSPLUGIN
               QAXSERVER QAXCONTAINER QTSCRIPT QTSVG QTUITOOLS QTHELP 
               QTWEBKIT PHONON QTSCRIPTTOOLS QTGUI QTTEST QTDBUS QTXML QTSQL 
               QTXMLPATTERNS QTNETWORK QTCORE)

  IF (QT_USE_${module} OR QT_USE_${module}_DEPENDS)
    IF (QT_${module}_FOUND)
      IF(QT_USE_${module})
        STRING(REPLACE "QT" "" qt_module_def "${module}")
        ADD_DEFINITIONS(-DQT_${qt_module_def}_LIB)
        INCLUDE_DIRECTORIES(SYSTEM ${QT_${module}_INCLUDE_DIR})
      ENDIF(QT_USE_${module})
      SET(QT_LIBRARIES ${QT_LIBRARIES} ${QT_${module}_LIBRARY} ${QT_${module}_LIB_DEPENDENCIES})
      FOREACH(depend_module ${QT_${module}_MODULE_DEPENDS})
        SET(QT_USE_${depend_module}_DEPENDS 1)
      ENDFOREACH(depend_module ${QT_${module}_MODULE_DEPENDS})
    ELSE (QT_${module}_FOUND)
      MESSAGE("Qt ${module} library not found.")
    ENDIF (QT_${module}_FOUND)
  ENDIF (QT_USE_${module} OR QT_USE_${module}_DEPENDS)
  
ENDFOREACH(module)

