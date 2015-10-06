#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {

/** @return a vector with all possible ReflectionCondition objects */
std::vector<ReflectionCondition_sptr> getAllReflectionConditions() {
  std::vector<ReflectionCondition_sptr> out;
  out.push_back(ReflectionCondition_sptr(new ReflectionConditionPrimitive()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionCFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionAFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionBFaceCentred()));
  out.push_back(ReflectionCondition_sptr(new ReflectionConditionBodyCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionAllFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionRhombohedrallyObverse()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionRhombohedrallyReverse()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionHexagonallyReverse()));
  return out;
}

} // namespace Mantid
} // namespace Geometry
