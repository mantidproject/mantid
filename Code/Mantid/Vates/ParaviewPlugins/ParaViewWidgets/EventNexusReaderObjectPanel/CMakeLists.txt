#Include Qt widgets
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets)
set_mantid_subprojects(Vates/VatesAPI)
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

#So that source file shows in VS.
SET(INCLUDE_FILES EventNexusReaderObjectPanel.h)
SET(SRC_FILES EventNexusReaderObjectPanel.cxx)

QT4_WRAP_CPP(MOC_SRCS EventNexusReaderObjectPanel.h)
ADD_PARAVIEW_OBJECT_PANEL(IFACES IFACE_SRCS 
                         CLASS_NAME EventNexusReaderObjectPanel
                         XML_NAME EventNexusReader XML_GROUP sources)
ADD_PARAVIEW_PLUGIN(MantidParaViewEventNexusReaderObjectPanel "1.0"
                    GUI_INTERFACES ${IFACES}
                    SOURCES ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} ${SRC_FILES})
TARGET_LINK_LIBRARIES(MantidParaViewEventNexusReaderObjectPanel MantidParaViewQtWidgets)
