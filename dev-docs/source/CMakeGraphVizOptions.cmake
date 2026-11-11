# Include Mantid executable and library targets.
set(GRAPHVIZ_EXECUTABLES TRUE)
set(GRAPHVIZ_STATIC_LIBS TRUE)
set(GRAPHVIZ_SHARED_LIBS TRUE)
set(GRAPHVIZ_MODULE_LIBS TRUE)
set(GRAPHVIZ_INTERFACE_LIBS TRUE)

# Exclude CMake OBJECT-library targets. Individual object files are not represented as nodes in CMake's Graphviz output.
set(GRAPHVIZ_OBJECT_LIBS FALSE)

# Show only targets owned by the Mantid project, omitting linked third-party and otherwise unknown external libraries.
set(GRAPHVIZ_EXTERNAL_LIBS FALSE)
set(GRAPHVIZ_UNKNOWN_LIBS FALSE)

# Limit output to executable and library target dependencies.
set(GRAPHVIZ_CUSTOM_TARGETS FALSE)

# Generate per-target dependency graphs, but not reverse-dependency graphs.
set(GRAPHVIZ_GENERATE_PER_TARGET TRUE)
set(GRAPHVIZ_GENERATE_DEPENDERS FALSE)
