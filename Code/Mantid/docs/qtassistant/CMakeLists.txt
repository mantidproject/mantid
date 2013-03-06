######################################################################
# QT assistant documentation generator
######################################################################
#QT_QCOLLECTIONGENERATOR_EXECUTABLE
if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   # set directories to follow structure of qt4 on linux
   set (HELP_OUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/qtassistant)
   set (HELP_QCH_DIR ${HELP_OUT_DIR}/qch)
   set (HELP_HTML_DIR ${HELP_OUT_DIR}/html)
   set (HELP_IMG_DIR ${HELP_OUT_DIR}/src/images)

   message (STATUS "Adding information for qtassistant")
   set (HELP_QHCP_SOURCE 
         ${HELP_OUT_DIR}/mantid.qhcp
         ${HELP_IMG_DIR}/Mantid_Logo_Transparent.png
         ${HELP_IMG_DIR}/MantidPlot_Icon_32offset.png
         ${HELP_OUT_DIR}/about.txt
         ${HELP_OUT_DIR}/mantidgeneral.qhp
        )

   add_custom_command( OUTPUT ${HELP_IMG_DIR}/Mantid_Logo_Transparent.png
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/img/Mantid_Logo_Transparent.png
                               ${HELP_IMG_DIR}/Mantid_Logo_Transparent.png
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/img/Mantid_Logo_Transparent.png )
   add_custom_command( OUTPUT ${HELP_IMG_DIR}/MantidPlot_Icon_32offset.png
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/img/MantidPlot_Icon_32offset.png
                               ${HELP_IMG_DIR}/MantidPlot_Icon_32offset.png
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/img/MantidPlot_Icon_32offset.png )
   add_custom_command( OUTPUT ${HELP_OUT_DIR}/mantidgeneral.qhp
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/src/mantidgeneral.qhp
                               ${HELP_OUT_DIR}/mantidgeneral.qhp )
   add_custom_command( OUTPUT ${HELP_OUT_DIR}/about.txt
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/src/about.txt
                               ${HELP_OUT_DIR}/about.txt )

   add_custom_command( OUTPUT ${HELP_OUT_DIR}/mantid.qhcp
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/mantid.qhcp
                               ${HELP_OUT_DIR}/mantid.qhcp )

   set ( HELP_ALGO_OUT
         ${HELP_OUT_DIR}/algorithms.qhp
         ${HELP_HTML_DIR}/algorithms_index.html )

   set ( HELP_FIT_OUT
         ${HELP_OUT_DIR}/fitfunctions.qhp
         ${HELP_HTML_DIR}/fitfunctions_index.html )

   add_custom_command(OUTPUT ${HELP_ALGO_OUT}
                      DEPENDS make_algorithms_help.py qhpfile.py algorithm_help.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_algorithms_help.py
                              -m ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR} -o ${HELP_OUT_DIR}
		      )

   add_custom_command(OUTPUT ${HELP_FIT_OUT}
                      DEPENDS make_fitfunctions_help.py qhpfile.py fitfunctions_help.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_fitfunctions_help.py
                              -m ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR} -o ${HELP_OUT_DIR}
		      )

   add_custom_command(OUTPUT  ${HELP_QCH_DIR}/mantid.qhc ${HELP_QCH_DIR}/algorithms.qch ${HELP_QCH_DIR}/fitfunctions.qch ${HELP_QCH_DIR}/mantidgeneral.qch
                      DEPENDS ${HELP_ALGO_OUT} ${HELP_FIT_OUT} ${HELP_QHCP_SOURCE}
		      COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${HELP_OUT_DIR}/mantid.qhcp
		      )

   add_custom_target ( qtassistant ALL
                       DEPENDS MantidPlot ${HELP_QCH_DIR}/mantid.qhc
                       )

#   install ( FILES mantid.qhc algorithms.qch mantidgeneral.qch
#             DESTINATION ${GUI_HELP_DEST} <- TODO define this variable
#             COMPONENT help )             <- TODO define this component

else (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (FATAL_ERROR " Did not find qcollectiongenerator - cannot create qtassistant files")
endif()
