# Developer documentation

# targets
set(BUILDER html)
set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BUILDER})
set(DOCTREE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctree)

# We try to execute Sphinx directly through python -m to avoid problems with the
# startup scripts on Windows. They are not always reliable as they can have
# hardcoded paths in them.
add_custom_target(dev-docs-${BUILDER}
                  COMMAND ${Python_EXECUTABLE} -m ${SPHINX_MAIN} ${SPHINX_NO_COLOR}
                          -w dev_docs_warnings.txt
                          -W --keep-going -b ${BUILDER}
                          -d ${DOCTREE_DIR} ${CMAKE_CURRENT_LIST_DIR}/source
                          ${OUT_DIR}
                  COMMENT "Building html developer documentation")

# Group within VS and exclude from whole build
set_target_properties(dev-docs-html
                      PROPERTIES FOLDER
                                 "Documentation"
                                 EXCLUDE_FROM_DEFAULT_BUILD
                                 1
                                 EXCLUDE_FROM_ALL
                                 1)
