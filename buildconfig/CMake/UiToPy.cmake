# ######################################################################################################################
# Function to convert ui files into PyQt5 python files
# ######################################################################################################################

function(UiToPy5 ui_files target_name)
  set(ui_dir ${CMAKE_CURRENT_SOURCE_DIR})
  set(_outputs "")
  foreach(ui_file ${${ui_files}})
    # Get the filename
    get_filename_component(ui_name ${ui_file} NAME_WE)
    # Generated file to create
    set(generated_file ${ui_dir}/ui_${ui_name}.py)
    # Source file to generate from
    set(source_file ${ui_dir}/${ui_name}.ui)
    # Command to run the translation
    add_custom_command(
      OUTPUT ${generated_file}
      COMMAND ${PYTHON_EXECUTABLE} -m PyQt5.uic.pyuic -x ${source_file} -o ${generated_file}
      DEPENDS ${source_file}
    )
    # Record all generated files
    list(APPEND _outputs ${generated_file})

  endforeach(ui_file)
  # Create a custom target
  add_custom_target(${target_name} DEPENDS ${_outputs})
  if(USE_PRIVATE_SIPPYQT5)
    add_dependencies(${target_name} extern-pyqt5)
  endif()
endfunction(UiToPy5)
