######################################################################
# QT assistant documentation generator
######################################################################
#QT_QCOLLECTIONGENERATOR_EXECUTABLE
if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
   message (STATUS "Adding information for qtassistant: mantid.qhc")
   set (HELP_QHCP_SOURCE 
         mantid.qhcp
         #generated/algorithms_index.html
         #generated/algorithms.qhp
         img/Mantid_Logo_Transparent.png
         img/MantidPlot_Icon_32offset.png
         src/about.txt
         #src/fitfunctions.html
         src/mantidgeneral.qhp
        )

   add_custom_command(OUTPUT generated/algorithms_index.html generated/algorithms.qhp
                      DEPENDS make_algorithms_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_algorithms_help.py -m ${CMAKE_BINARY_DIR}
		      )

   add_custom_command(OUTPUT generated/fitfunctions_index.html generated/fitfunctions.qhp
                      DEPENDS make_fitfunctions_help.py qhpfile.py
                      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_fitfunctions_help.py -m ${CMAKE_BINARY_DIR}
		      )

   add_custom_command(OUTPUT  mantid.qhc algorithms.qch mantidgeneral.qch
                      DEPENDS generated/algorithms_index.html generated/algorithms.qhp generated/fitfunctions_index.html generated/fitfunctions.qhp ${HELP_QHCP_SOURCE}
		      COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mantid.qhcp
		      )

   add_custom_target ( qtassistant
                       DEPENDS MantidPlot generated/algorithms.qhp generated/fitfunctions.qhp mantid.qhc
                       )

#   install ( FILES mantid.qhc algorithms.qch mantidgeneral.qch
#             DESTINATION ${GUI_HELP_DEST} <- TODO define this variable
#             COMPONENT help )             <- TODO define this component

endif()
