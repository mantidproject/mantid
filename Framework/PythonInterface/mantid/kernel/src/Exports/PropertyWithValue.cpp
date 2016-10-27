#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"

#include <boost/python/copy_const_reference.hpp>

using Mantid::PythonInterface::PropertyWithValueExporter;

void export_BasicPropertyWithValueTypes() {
  // ints32 & vectors
  PropertyWithValueExporter<int32_t>::define("Int32PropertyWithValue");
  PropertyWithValueExporter<std::vector<int32_t>>::define(
      "VectorInt32PropertyWithValue");
  PropertyWithValueExporter<uint32_t>::define("UInt32PropertyWithValue");
  PropertyWithValueExporter<std::vector<uint32_t>>::define(
      "VectorUInt32PropertyWithValue");

  // ints64 & vectors
  PropertyWithValueExporter<int64_t>::define("Int64PropertyWithValue");
  PropertyWithValueExporter<std::vector<int64_t>>::define(
      "VectorInt64PropertyWithValue");
  PropertyWithValueExporter<uint64_t>::define("UInt64PropertyWithValue");
  PropertyWithValueExporter<std::vector<uint64_t>>::define(
      "VectorUInt64PropertyWithValue");

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
