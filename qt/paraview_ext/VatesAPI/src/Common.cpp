#include "MantidVatesAPI/Common.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/UnitLabel.h"

#include <vtkFieldData.h>
#include <vtkNew.h>
#include <vtkStringArray.h>

#include "vtkPVChangeOfBasisHelper.h"

#include <boost/regex.hpp>

// using namespace Mantid::Geometry;
namespace Mantid {
namespace VATES {
std::string makeAxisTitle(const Mantid::Geometry::IMDDimension &dim) {
  // The UnitLabels which are stored in old files don't necessarily contain
  // valid Latex symbols. We check if there is a difference between the ASCII
  // and the Latex symbols, if there is one, then use the Latex else modify
  // the ASCII version to display Latex

  auto latexSymbol = dim.getMDUnits().getUnitLabel().latex();
  auto asciiSymbol = dim.getMDUnits().getUnitLabel().ascii();
  auto hasLatexSymbol = asciiSymbol != latexSymbol;
  std::string symbol;

  if (hasLatexSymbol) {
    symbol = latexSymbol;
  } else {
    symbol = convertAxesTitleToLatex(latexSymbol);
  }

  std::string title = dim.getName();
  title += " ($";
  title += symbol;
  title += "$)";
  return title;
}

std::string convertAxesTitleToLatex(const std::string &toConvert) {
  std::string converted;
  // Check if the input has a unit of A\\^-1: this is converted to \\\\AA^{-1}
  // else
  // Check if the input has a unit of Ang: this is convered to \\\\AA
  // else leave as it is
  if (toConvert.find("A^-1") != std::string::npos) {
    boost::regex re("A\\^-1");
    converted = boost::regex_replace(toConvert, re, "\\\\AA^{-1}");
  } else if (toConvert.find("Ang") != std::string::npos) {
    boost::regex re("Ang");
    converted = boost::regex_replace(toConvert, re, "\\\\AA");
  } else {
    converted = toConvert;
  }

  // Finally if there are any spaces they will disappear in Mathmode, hence we
  // need to replace
  // any space with $ $.
  boost::regex re(" ");
  converted = boost::regex_replace(converted, re, "$ $");

  return converted;
}

void setAxisLabel(const std::string &metadataLabel,
                  const std::string &labelString, vtkFieldData *fieldData) {
  vtkNew<vtkStringArray> axisTitle;
  axisTitle->SetName(metadataLabel.c_str());
  axisTitle->SetNumberOfComponents(1);
  axisTitle->SetNumberOfTuples(1);
  axisTitle->SetValue(0, labelString.c_str());
  fieldData->AddArray(axisTitle.GetPointer());
}

} // namespace VATES
} // namespace Mantid
