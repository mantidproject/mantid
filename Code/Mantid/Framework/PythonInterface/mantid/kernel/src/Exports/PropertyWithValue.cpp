#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"

using Mantid::PythonInterface::PropertyWithValueExporter;

// clang-format off
void export_BasicPropertyWithValueTypes()
// clang-format on
{
  // cut down copy-and-paste code
#define EXPORT_PROP(CType, ExportName) \
  PropertyWithValueExporter<CType>::define(ExportName);

  //ints & vectors
  EXPORT_PROP(int, "IntPropertyWithValue");
  EXPORT_PROP(std::vector<int>, "VectorIntPropertyWithValue");
  EXPORT_PROP(unsigned int, "UIntPropertyWithValue");
  EXPORT_PROP(std::vector<unsigned int>, "VectorUIntPropertyWithValue");
  // longs & vectors
  EXPORT_PROP(long, "LongPropertyWithValue");
  EXPORT_PROP(std::vector<long>, "VectorLongPropertyWithValue");
  EXPORT_PROP(unsigned long, "ULongPropertyWithValue");
  EXPORT_PROP(std::vector<unsigned long>, "VectorULongPropertyWithValue");
  // long long long longs & vectors
  EXPORT_PROP(long long, "LongLongPropertyWithValue");
  EXPORT_PROP(std::vector<long long>, "VectorLongLongPropertyWithValue");
  EXPORT_PROP(unsigned long long, "ULongLongPropertyWithValue");
  EXPORT_PROP(std::vector<unsigned long long>, "VectorULongLongPropertyWithValue");
  // double
  EXPORT_PROP(double, "FloatPropertyWithValue");
  EXPORT_PROP(std::vector<double>, "VectorFloatPropertyWithValue");
  // boolean
  EXPORT_PROP(bool, "BoolPropertyWithValue");
  EXPORT_PROP(std::vector<bool>, "VectorBoolPropertyWithValue");
  // std::string
  EXPORT_PROP(std::string, "StringPropertyWithValue");
  EXPORT_PROP(std::vector<std::string>, "VectorStringPropertyWithValue");


#undef EXPORT_PROP
}
