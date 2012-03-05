#include "MantidPythonInterface/kernel/PropertyWithValue.h"

void export_BasicPropertyWithValueTypes()
{
  // See MantidPythonInterface/kernel/PropertyWithValue.h for macro definition
  EXPORT_PROP_W_VALUE(int32_t, int32_t);
  EXPORT_PROP_W_VALUE(std::vector<int32_t>, vector_int32_t);
  EXPORT_PROP_W_VALUE(int64_t, int64_t);
  EXPORT_PROP_W_VALUE(std::vector<int64_t>, vector_int64_t);

  EXPORT_PROP_W_VALUE(size_t, size_t);
  EXPORT_PROP_W_VALUE(std::vector<size_t>, vector_size_t);

  EXPORT_PROP_W_VALUE(double, dbl);
  EXPORT_PROP_W_VALUE(std::vector<double>,vector_dbl);

  EXPORT_PROP_W_VALUE(bool, bool);

  EXPORT_PROP_W_VALUE(std::string, string);
  EXPORT_PROP_W_VALUE(std::vector<std::string>,vector_str);
}


