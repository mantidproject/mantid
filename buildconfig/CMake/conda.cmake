set(CONDA_WORKDIR "conda-packaging")
configure_file(buildconfig/CMake/conda-update-recipe.py.in ${CONDA_WORKDIR}/conda-update-recipe.py)
add_custom_target(
  conda-update-recipe
  COMMAND python conda-update-recipe.py
  WORKING_DIRECTORY ${CONDA_WORKDIR}
  )
