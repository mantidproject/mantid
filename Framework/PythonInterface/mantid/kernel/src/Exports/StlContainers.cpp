#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

using Mantid::PythonInterface::std_set_exporter;
using Mantid::PythonInterface::std_vector_exporter;

void exportStlContainers() {
  // Export some frequently used stl containers
  // std::vector
  std_vector_exporter<int>::wrap("std_vector_int");
  std_vector_exporter<long>::wrap("std_vector_long");
  std_vector_exporter<std::size_t>::wrap("std_vector_size_t");
  std_vector_exporter<double>::wrap("std_vector_dbl");
  std_vector_exporter<bool>::wrap("std_vector_bool");
  std_vector_exporter<std::string>::wrap("std_vector_str");
  std_vector_exporter<Mantid::Types::Core::DateAndTime>::wrap(
      "std_vector_dateandtime");
  std_vector_exporter<Mantid::Kernel::Quat>::wrap("std_vector_quat");
  std_vector_exporter<Mantid::Kernel::V3D>::wrap("std_vector_v3d");
  // std::set
  std_set_exporter<int>::wrap("std_set_int");
  std_set_exporter<std::string>::wrap("std_set_str");
}
