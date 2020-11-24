set(SRC_FILES
    src/FitScriptGeneratorModel.cpp
    src/FitScriptGeneratorPresenter.cpp
    src/FitScriptGeneratorView.cpp
)

set(INC_FILES
    inc/DllOption.h
    inc/FitScriptGeneratorModel.h
    inc/FitScriptGeneratorPresenter.h
    inc/FitScriptGeneratorView.h
)

set(MOC_FILES
    inc/FitScriptGeneratorView.h
)

set(UI_FILES
    inc/FitScriptGenerator.ui
)

mtd_add_qt_library(
    TARGET_NAME MantidQtWidgetsFitting
    QT_VERSION 5
    SRC ${SRC_FILES}
    MOC ${MOC_FILES}
    NOMOC ${INC_FILES}
    UI ${UI_FILES}
    DEFS
        IN_MANTIDQT_FITTING
    INCLUDE_DIRS
        inc
    SYSTEM_INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
    LINK_LIBS
        ${TARGET_LIBRARIES}
        PUBLIC
        PythonInterfaceCore
    MTD_QT_LINK_LIBS
        MantidQtIcons
    INSTALL_DIR ${WORKBENCH_LIB_DIR}
    OSX_INSTALL_RPATH
        @loader_path/../MacOS
        @loader_path/../Frameworks
    LINUX_INSTALL_RPATH "\$ORIGIN/../${WORKBENCH_LIB_DIR}"
)