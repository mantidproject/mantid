function(UiToPy py_exec py_uic_py ui_files)

  
  set(ui_dir ${CMAKE_CURRENT_SOURCE_DIR})
  foreach(ui_file ${${ui_files}})
    # Establish the parent directory
    get_filename_component(ui_name ${ui_file}  NAME_WE)
    execute_process(COMMAND ${py_exec} ${py_uic_py} ${ui_dir}/${ui_name}.ui -o ${ui_dir}/${ui_name}.py)
  endforeach(ui_file)
endfunction(UiToPy)