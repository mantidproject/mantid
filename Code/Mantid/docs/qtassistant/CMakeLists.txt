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
         src/algorithms.html
         src/algorithms.qhp
         src/fitfunctions.html
         src/mantidgeneral.qhp
        )

   add_custom_command(OUTPUT  mantid.qhc algorithms.qch mantidgeneral.qch
                      DEPENDS ${HELP_QHCP_SOURCE}
		      COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mantid.qhcp
		      )

   add_custom_target ( qtassistant ALL
                       DEPENDS mantid.qhc MantidPlot)

#   install ( FILES mantid.qhc algorithms.qch mantidgeneral.qch
#             DESTINATION ${GUI_HELP_DEST} <- TODO define this variable
#             COMPONENT help )             <- TODO define this component

endif()
