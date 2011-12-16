#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

using Mantid::PythonInterface::std_vector_exporter;
using Mantid::PythonInterface::std_set_exporter;

void exportStlContainers()
{
    // Export some frequently used stl containers
  // std::vector
  std_vector_exporter<int>::wrap("std_vector_int");
  std_vector_exporter<long>::wrap("std_vector_long");
  std_vector_exporter<std::size_t>::wrap("std_vector_size_t");
  std_vector_exporter<double>::wrap("std_vector_dbl");
  std_vector_exporter<bool>::wrap("std_vector_bool");
  std_vector_exporter<std::string>::wrap("std_vector_str");

  //std::set
  std_set_exporter<int>::wrap("std_set_int");
}
