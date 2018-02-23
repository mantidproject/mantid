###############################################################################
# Developer documentation
###############################################################################
# targets
set ( BUILDER html )
set ( OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BUILDER} )
set ( DOCTREE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctree )

add_custom_target ( dev-docs-${BUILDER}
                   COMMAND python -m sphinx -b ${BUILDER} -d ${DOCTREE_DIR} ${CMAKE_CURRENT_LIST_DIR}/source ${OUT_DIR}
                   COMMENT "Building html developer documentation" )
# Group within VS and exclude from whole build
set_target_properties ( dev-docs-html PROPERTIES FOLDER "Documentation"
                       EXCLUDE_FROM_DEFAULT_BUILD 1
                       EXCLUDE_FROM_ALL 1 )
