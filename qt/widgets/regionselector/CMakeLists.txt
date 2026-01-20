# Rules for matplotlib cpp library
set(LIB_SRCS src/RegionSelector.cpp)

set(NOMOC_HEADERS inc/MantidQtWidgets/RegionSelector/IRegionSelector.h
                  inc/MantidQtWidgets/RegionSelector/RegionSelector.h
)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsRegionSelector
  QT_VERSION 5
  SRC ${LIB_SRCS}
  NOMOC ${NOMOC_HEADERS}
  DEFS IN_MANTIDQT_REGIONSELECTOR
  INCLUDE_DIRS inc ${Boost_INCLUDE_DIRS}
  LINK_LIBS Mantid::PythonInterfaceCore Mantid::API
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../MacOS @loader_path/../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}"
)

# Auto-generate exports header
target_include_directories(
  MantidQtWidgetsRegionSelector-Qt5 PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/qt/widgets/regionselector> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(MantidQtWidgetsRegionSelector TRUE)
