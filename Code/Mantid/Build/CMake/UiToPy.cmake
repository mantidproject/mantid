###########################################################################
# Function to convert ui files into PyQt4 python files
###########################################################################

function(UiToPy ui_files)
  set(py_exec ${PYTHON_EXECUTABLE})
  set(py_uic_py ${PYQT4_PYUIC} ) # From FindPyQt4
  set(ui_dir ${CMAKE_CURRENT_SOURCE_DIR})
  foreach(ui_file ${${ui_files}})
    # Get the filename
    get_filename_component(ui_name ${ui_file}  NAME_WE)
    # Run the converter
    execute_process(COMMAND ${py_exec} ${py_uic_py} ${ui_dir}/${ui_name}.ui -o ${ui_dir}/ui_${ui_name}.py)
  endforeach(ui_file)
endfunction(UiToPy)