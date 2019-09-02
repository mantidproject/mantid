// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/PlotAxis.h"
#include "MantidQtWidgets/Common/QStringUtils.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace API {

namespace {
QString
replacePerWithNegativeIndice(const std::string &label,
                             const bool &plotAsDistribution,
                             const Mantid::Kernel::UnitLabel xLabel = "") {
  std::vector<std::string> splitVec;
  QString negativeOnePower = toQStringInternal(L"\u207b\u00b9");
  QString newLabel;

  boost::split_regex(splitVec, label, boost::regex(" per "));
  if (splitVec.size() > 1) {
    newLabel = QString::fromStdString(splitVec[0]);
    splitVec.erase(splitVec.begin());
    std::string unitString = boost::algorithm::join(splitVec, " ");
    newLabel += " (" + QString::fromStdString(unitString);
    if (plotAsDistribution && xLabel != "") {
      newLabel += " " + toQStringInternal(xLabel.utf8());
    }
    newLabel += ")" + negativeOnePower;
  } else {
    newLabel = QString::fromStdString(label);
    if (plotAsDistribution && xLabel != "") {
      newLabel +=
          " (" + toQStringInternal(xLabel.utf8()) + ")" + negativeOnePower;
    }
  }
  return newLabel;
}
} // namespace
/**
 * The title will be filled with the caption & units from the given Axis object
 * of the workspace
 * @param workspace The workspace containing the axis information
 * @param index Index of the axis in the workspace to inspect
 */
PlotAxis::PlotAxis(const Mantid::API::IMDWorkspace &workspace,
                   const size_t index)
    : m_title() {
  if (index < workspace.getNumDims())
    titleFromIndex(workspace, index);
  else
    throw std::invalid_argument("PlotAxis() - Unknown axis index: '" +
                                boost::lexical_cast<std::string>(index) + "'");
}

/**
 * The title will be filled with the caption & units from the given
 * dimension object
 * @param dim A dimension object, usually from a workspace
 */
PlotAxis::PlotAxis(const Mantid::Geometry::IMDDimension &dim) {
  titleFromDimension(dim);
}

/**
 * The title will be filled with the caption & label for the Y data values
 * within the workspace, ~ the Z axis
 * @param plottingDistribution If true, the Y axis has been divided by the bin
 * width
 * @param workspace The workspace containing the Y title information
 */
PlotAxis::PlotAxis(const bool plottingDistribution,
                   const Mantid::API::MatrixWorkspace &workspace) {
  titleFromYData(workspace, plottingDistribution);
}

/**
 * @return A new title for this axis
 */
QString PlotAxis::title() const { return m_title; }

/**
 * @param workspace  The workspace containing the axis information
 * @param index Index of the axis in the workspace to inspect
 */
void PlotAxis::titleFromIndex(const Mantid::API::IMDWorkspace &workspace,
                              const size_t index) {
  auto dim = workspace.getDimension(index);
  titleFromDimension(*dim);
  if (m_title.isEmpty()) {
    m_title = (index == 0) ? "X axis" : "Y axis";
  }
}

/**
 * @param dim A dimension object that encapsulates the axis data
 */
void PlotAxis::titleFromDimension(const Mantid::Geometry::IMDDimension &dim) {
  m_title = QString::fromStdString(dim.getName());
  if (!m_title.isEmpty()) {
    auto unitLbl = dim.getUnits();
    if (!unitLbl.utf8().empty()) {
      m_title += " (" + toQStringInternal(unitLbl.utf8()) + ")";
    } else if (!unitLbl.ascii().empty()) {
      m_title += " (" + QString::fromStdString(unitLbl.ascii()) + ")";
    }
  }
}

/**
 * Constructs a title using the unicode methods of the UnitLabel
 * @param workspace The workspace containing the Y title information
 * @param plottingDistribution If true, the Y axis has been divided by the bin
 * width
 */
void PlotAxis::titleFromYData(const Mantid::API::MatrixWorkspace &workspace,
                              const bool plottingDistribution) {
  std::string yLabel = workspace.YUnitLabel();
  if (yLabel.empty()) {
    yLabel = workspace.YUnit();
  }
  if (!workspace.isDistribution() && plottingDistribution) {
    const auto xLabel = workspace.getAxis(0)->unit()->label();
    m_title = replacePerWithNegativeIndice(yLabel, true, xLabel);
  } else {
    m_title = replacePerWithNegativeIndice(yLabel, false);
  }
}

} // namespace API
} // namespace MantidQt
