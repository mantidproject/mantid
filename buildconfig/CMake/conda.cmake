set(CONDA_WORKDIR "conda-packaging")
configure_file(buildconfig/CMake/conda_update_recipe.py.in ${CONDA_WORKDIR}/conda_update_recipe.py)
add_custom_target(
  conda-update-recipe
  COMMAND python conda_update_recipe.py
  WORKING_DIRECTORY ${CONDA_WORKDIR}
)

# This creates a `.env` file for use in docker when building the conda package
configure_file(buildconfig/CMake/Packaging/docker_env.in ${CONDA_WORKDIR}/.env)
