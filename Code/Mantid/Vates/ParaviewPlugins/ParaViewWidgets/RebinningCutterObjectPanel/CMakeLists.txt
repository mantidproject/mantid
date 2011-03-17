
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets)
set_mantid_subprojects(Vates/VatesAPI)
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})
QT4_WRAP_CPP(MOC_SRCS RebinningCutterObjectPanel.h)
ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS 
                         CLASS_NAME RebinningCutterObjectPanel
                         XML_NAME RebinningCutter XML_GROUP filters)
ADD_PARAVIEW_PLUGIN(MantidParaViewRebinningCutterObjectPanel "1.0"
                    GUI_INTERFACES ${IFACES}
                    SOURCES ${MOC_SRCS} ${IFACE_SRCS})
TARGET_LINK_LIBRARIES(MantidParaViewRebinningCutterObjectPanel MantidParaViewQtWidgets)
