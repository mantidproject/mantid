#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

void exportStlContainers()
{
  using Mantid::PythonInterface::std_vector_exporter;

  // Export some frequently used stl containers
  std_vector_exporter<int>::wrap("std_vector_int");
  std_vector_exporter<long>::wrap("std_vector_long");
  std_vector_exporter<std::size_t>::wrap("std_vector_size_t");
  std_vector_exporter<double>::wrap("std_vector_dbl");
  std_vector_exporter<bool>::wrap("std_vector_bool");
  std_vector_exporter<std::string>::wrap("cpp_list_str");
}
