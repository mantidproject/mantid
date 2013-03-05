######################################################################
# QT assistant documentation generator
######################################################################
#QT_QCOLLECTIONGENERATOR_EXECUTABLE
if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (STATUS "Adding information for qtassistant: mantid.qhc")
   set (HELP_QHCP_SOURCE 
         ${CMAKE_BINARY_DIR}/qtassistant/mantid.qhcp
         img/Mantid_Logo_Transparent.png
         img/MantidPlot_Icon_32offset.png
         src/about.txt
         src/mantidgeneral.qhp
        )

   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/mantid.qhcp.template
                   ${CMAKE_BINARY_DIR}/qtassistant/mantid.qhcp )

   set ( HELP_ALGO_OUT
         ${CMAKE_BINARY_DIR}/qtassistant/generated/algorithms.qhp
         ${CMAKE_BINARY_DIR}/qtassistant/generated/algorithms_index.html )

   set ( HELP_FIT_OUT
         ${CMAKE_BINARY_DIR}/qtassistant/generated/fitfunctions.qhp
         ${CMAKE_BINARY_DIR}/qtassistant/generated/fitfunctions_index.html )

   add_custom_command(OUTPUT ${HELP_ALGO_OUT}
                      DEPENDS make_algorithms_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_algorithms_help.py
                              -m ${CMAKE_BINARY_DIR} -o ${CMAKE_BINARY_DIR}/qtassistant/generated
		      )

   add_custom_command(OUTPUT ${HELP_FIT_OUT}
                      DEPENDS make_fitfunctions_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_fitfunctions_help.py
                              -m ${CMAKE_BINARY_DIR} -o ${CMAKE_BINARY_DIR}/qtassistant/generated
		      )

   add_custom_command(OUTPUT  mantid.qhc algorithms.qch mantidgeneral.qch
                      DEPENDS ${HELP_ALGO_OUT} ${HELP_FIT_OUT} ${HELP_QHCP_SOURCE}
		      COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${CMAKE_BINARY_DIR}/qtassistant/mantid.qhcp
		      )

   add_custom_target ( qtassistant
                       DEPENDS MantidPlot
		               ${CMAKE_BINARY_DIR}/qtassistant/generated/algorithms.qhp
			       ${CMAKE_BINARY_DIR}/qtassistant/generated/fitfunctions.qhp
			       mantid.qhc
                       )

#   install ( FILES mantid.qhc algorithms.qch mantidgeneral.qch
#             DESTINATION ${GUI_HELP_DEST} <- TODO define this variable
#             COMPONENT help )             <- TODO define this component

else (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (FATAL_ERROR " Did not find qcollectiongenerator - cannot create qtassistant files")
endif()
