#include "MantidGeometry/Crystal/ReflectionGenerator.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace boost::python;

namespace {
std::vector<V3D> getHKLsDefaultFilter(ReflectionGenerator &self, double dMin,
                                      double dMax) {
  return self.getHKLs(dMin, dMax);
}

std::vector<V3D> getHKLsUsingFilter(ReflectionGenerator &self, double dMin,
                                    double dMax,
                                    ReflectionConditionFilter filter) {
  return self.getHKLs(dMin, dMax, self.getReflectionConditionFilter(filter));
}

std::vector<V3D> getUniqueHKLsDefaultFilter(ReflectionGenerator &self,
                                            double dMin, double dMax) {
  return self.getUniqueHKLs(dMin, dMax);
}

std::vector<V3D> getUniqueHKLsUsingFilter(ReflectionGenerator &self,
                                          double dMin, double dMax,
                                          ReflectionConditionFilter filter) {
  return self.getUniqueHKLs(dMin, dMax,
                            self.getReflectionConditionFilter(filter));
}
}

void export_ReflectionGenerator() {
  enum_<ReflectionConditionFilter>("ReflectionConditionFilter")
      .value("None", ReflectionConditionFilter::None)
      .value("Centering", ReflectionConditionFilter::Centering)
      .value("SpaceGroup", ReflectionConditionFilter::SpaceGroup)
      .value("StructureFactor", ReflectionConditionFilter::StructureFactor)
      .export_values();

  class_<ReflectionGenerator>("ReflectionGenerator", no_init)
      .def(init<const CrystalStructure &, optional<ReflectionConditionFilter>>(
          (arg("crystalStructure"), arg("defaultFilter"))))
      .def("getHKLs", &getHKLsDefaultFilter)
      .def("getHKLsUsingFilter", &getHKLsUsingFilter)
      .def("getUniqueHKLs", &getUniqueHKLsDefaultFilter)
      .def("getUniqueHKLsUsingFilter", &getUniqueHKLsUsingFilter)
      .def("getDValues", &ReflectionGenerator::getDValues)
      .def("getFsSquared", &ReflectionGenerator::getFsSquared);
}
