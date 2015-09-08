#include "MantidVatesAPI/Common.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/UnitLabel.h"

#include <vtkNew.h>
#include <vtkFieldData.h>
#include <vtkStringArray.h>

#include "vtkPVChangeOfBasisHelper.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/regex.hpp>

// using namespace Mantid::Geometry;
namespace Mantid {
namespace VATES {
std::string makeAxisTitle(Dimension_const_sptr dim) {
  std::string title = dim->getName();
  title += " (";
  title += dim->getUnits();
  title += ")";
  // update inverse angstrom symbol so it is rendered in mathtex.
  // Consider removing after ConvertToMD provides unit labels with latex
  // symbols.
  std::string angstromKeyword("Angstrom");
  boost::regex re;
  if (title.find(angstromKeyword) != std::string::npos) {
    re = boost::regex("Angstrom\\^-1");
  } else {
    re = boost::regex("A\\^-1");
  }
  return boost::regex_replace(title, re, "$\\\\AA^{-1}$");
}

void setAxisLabel(std::string metadataLabel, std::string labelString,
                  vtkFieldData *fieldData) {
  vtkNew<vtkStringArray> axisTitle;
  axisTitle->SetName(metadataLabel.c_str());
  axisTitle->SetNumberOfComponents(1);
  axisTitle->SetNumberOfTuples(1);
  axisTitle->SetValue(0, labelString.c_str());
  fieldData->AddArray(axisTitle.GetPointer());
}

bool isSpecial(double value) {
  return boost::math::isnan(value) || boost::math::isinf(value);
}

} // VATES
} // Mantid
