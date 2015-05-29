#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"

#include <boost/python/copy_const_reference.hpp>

using Mantid::PythonInterface::PropertyWithValueExporter;

// clang-format off
void export_BasicPropertyWithValueTypes()
// clang-format on
{
  // ints & vectors
  PropertyWithValueExporter<int>::define("IntPropertyWithValue");
  PropertyWithValueExporter<std::vector<int>>::define(
      "VectorIntPropertyWithValue");
  PropertyWithValueExporter<unsigned int>::define("UIntPropertyWithValue");
  PropertyWithValueExporter<std::vector<unsigned int>>::define(
      "VectorUIntPropertyWithValue");
  
  // longs & vectors
  PropertyWithValueExporter<long>::define("LongPropertyWithValue");
  PropertyWithValueExporter<std::vector<long>>::define(
      "VectorLongPropertyWithValue");
  PropertyWithValueExporter<unsigned long>::define("ULongPropertyWithValue");
  PropertyWithValueExporter<std::vector<unsigned long>>::define(
      "VectorULongPropertyWithValue");
  
  // long long long longs & vectors
  PropertyWithValueExporter<long long>::define("LongLongPropertyWithValue");
  PropertyWithValueExporter<std::vector<long long>>::define(
      "VectorLongLongPropertyWithValue");
  PropertyWithValueExporter<unsigned long long>::define(
      "ULongLongPropertyWithValue");
  PropertyWithValueExporter<std::vector<unsigned long long>>::define(
      "VectorULongLongPropertyWithValue");
  
  // double
  PropertyWithValueExporter<double>::define("FloatPropertyWithValue");
  PropertyWithValueExporter<std::vector<double>>::define(
      "VectorFloatPropertyWithValue");
  
  // boolean
  PropertyWithValueExporter<bool>::define("BoolPropertyWithValue");
  PropertyWithValueExporter<std::vector<bool>>::define(
      "VectorBoolPropertyWithValue");
  
  // std::string
  PropertyWithValueExporter<std::string>::define("StringPropertyWithValue");
  PropertyWithValueExporter<std::vector<std::string>>::define(
      "VectorStringPropertyWithValue");
}
