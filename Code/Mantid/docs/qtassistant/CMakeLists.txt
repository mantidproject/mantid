######################################################################
# QT assistant documentation generator
######################################################################
#QT_QCOLLECTIONGENERATOR_EXECUTABLE
if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (STATUS "Adding information for qtassistant: mantid.qhc")
   set (HELP_QHCP_SOURCE 
         mantid.qhcp
         img/Mantid_Logo_Transparent.png
         img/MantidPlot_Icon_32offset.png
         src/about.txt
         src/mantidgeneral.qhp
        )

   set ( HELP_ALGO_OUT
         generated/algorithms.qhp
         generated/algorithms_index.html )

   set ( HELP_FIT_OUT
         generated/fitfunctions.qhp
         generated/fitfunctions_index.html )


   add_custom_command(OUTPUT ${HELP_ALGO_OUT}
                      DEPENDS make_algorithms_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_algorithms_help.py -m ${CMAKE_BINARY_DIR}
		      )

   add_custom_command(OUTPUT ${HELP_FIT_OUT}
                      DEPENDS make_fitfunctions_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_fitfunctions_help.py -m ${CMAKE_BINARY_DIR}
		      )

   add_custom_command(OUTPUT  mantid.qhc algorithms.qch mantidgeneral.qch
                      DEPENDS ${HELP_ALGO_OUT} ${HELP_FIT_OUT} ${HELP_QHCP_SOURCE}
		      COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mantid.qhcp
		      )

   add_custom_target ( qtassistant
                       DEPENDS MantidPlot generated/algorithms.qhp generated/fitfunctions.qhp mantid.qhc
                       )

#   install ( FILES mantid.qhc algorithms.qch mantidgeneral.qch
#             DESTINATION ${GUI_HELP_DEST} <- TODO define this variable
#             COMPONENT help )             <- TODO define this component

else (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (status " Did not find QCOLLECTIONGENERATOR - cannot create qtassistant files")
endif()
