// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/DisplayCurveFit.h"
#include "MantidKernel/Logger.h"
// includes for workspace handling

// includes for interface development
#include "MantidQtWidgets/Plotting/Qwt/RangeSelector.h"

namespace {
Mantid::Kernel::Logger g_log("DisplayCurveFit");
}

namespace MantidQt {
namespace MantidWidgets {

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Public members ++++++++
//               ++++++++++++++++++++++++++++++++

/// Curve types have a hard-coded name
std::map<DisplayCurveFit::curveType, QString> const
    DisplayCurveFit::m_curveTypeToQString{
        {DisplayCurveFit::curveType::data, QString("data")},
        {DisplayCurveFit::curveType::guess, QString("guess")},
        {DisplayCurveFit::curveType::fit, QString("fit")},
        {DisplayCurveFit::curveType::residuals, QString("residuals")}};

/// Curves should be recognizable by their color
std::map<DisplayCurveFit::curveType, Qt::GlobalColor> const
    DisplayCurveFit::m_curveTypeToColor{
        {DisplayCurveFit::curveType::data, Qt::black},
        {DisplayCurveFit::curveType::guess, Qt::blue},
        {DisplayCurveFit::curveType::fit, Qt::red},
        {DisplayCurveFit::curveType::residuals, Qt::red}};

/// Range types have a hard-coded name
std::map<DisplayCurveFit::dcRange, QString> const
    DisplayCurveFit::m_dcRangeToQString{
        {DisplayCurveFit::dcRange::fit, QString("fit")},
        {DisplayCurveFit::dcRange::evaluate, QString("evaluate")}};

/// Constructor
DisplayCurveFit::DisplayCurveFit(QWidget *parent)
    : API::MantidWidget(parent), m_rangeSelector() {
  m_uiForm.setupUi(this); // First instantiate widgets components
  m_plotPanel = {{curveType::data, m_uiForm.fitPlot},
                 {curveType::guess, m_uiForm.fitPlot},
                 {curveType::fit, m_uiForm.fitPlot},
                 {curveType::residuals, m_uiForm.residualsPlot}};
}

DisplayCurveFit::~DisplayCurveFit() {}

/**
 * @brief Sets the range of the given axis scale to a given range in the two
 * fitting
 * panels.
 * @param range Pair of values for range
 * @param axisID ID of axis
 * @exception std::runtime_error if supplied range is nonsense
 */
void DisplayCurveFit::setAxisRange(QPair<double, double> range, int axisID) {
  m_uiForm.fitPlot->setAxisRange(range, axisID);
  m_uiForm.residualsPlot->setAxisRange(range, axisID);
}

/**
 * @brief Finds which stored fitting curves are associated with the query
 * workspace
 * @param workspace Pointer to workspace
 * @return a std::vector containing the curve types
 */
DisplayCurveFit::curveTypes DisplayCurveFit::getCurvesForWorkspace(
    const Mantid::API::MatrixWorkspace_sptr workspace) {
  QStringList curveNames = m_uiForm.fitPlot->getCurvesForWorkspace(workspace);
  curveNames =
      curveNames + m_uiForm.residualsPlot->getCurvesForWorkspace(workspace);
  return this->namesToTypes(curveNames);
}

/**
 * @brief Gets the X range of a curve given its curve type.
 * @param atype the query curve type.
 * @exception std::runtime_error when there is no associated curve in any
 * of the two PreviewPlot  panels.
 * @return a std::pair definining the range of the curve.
 */
QPair<double, double> DisplayCurveFit::getCurveRange(const curveType &atype) {
  return m_plotPanel.at(atype)->getCurveRange(m_curveTypeToQString.at(atype));
}

/**
 * @brief Gets the X range of the first curve whose data is stored in the query
 * workspace.
 * @param workspace pointer to query workspace
 * @return the range of the first curve associated to the workspace
 * @exception std::runtime_error no stored curves are associated to the query
 * workspace
 */
QPair<double, double> DisplayCurveFit::getCurveRange(
    const Mantid::API::MatrixWorkspace_sptr workspace) {
  curveTypes typesFound = this->getCurvesForWorkspace(workspace);
  if (typesFound.size() == 0) {
    throw std::runtime_error("No fitting curves associated to workspace" +
                             workspace->getName());
  }
  return getCurveRange(typesFound[0]);
}

/**
 * @brief Inserts one of the curves.
 * @param aType type of the curve to be inserted.
 * @param workspace Pointer to the workspace holding the data to plot the curve.
 * @param specIndex Spectrum index of workspace argument.
 */
void DisplayCurveFit::addSpectrum(
    const curveType &aType, const Mantid::API::MatrixWorkspace_sptr workspace,
    const size_t specIndex) {
  const QString &curveName{m_curveTypeToQString.at(aType)};
  const QColor curveColor(m_curveTypeToColor.at(aType));
  m_plotPanel.at(aType)->addSpectrum(curveName, workspace, specIndex,
                                     curveColor);
}

/**
 * @brief Removes the curve of the query type
 * @param aType the curve type to remove
 */
void DisplayCurveFit::removeSpectrum(const curveType &aType) {
  m_plotPanel.at(aType)->removeSpectrum(m_curveTypeToQString.at(aType));
}

/**
 * @brief Checks to see if a given curve type is present in any
 * of the PreviewPlot panels.
 * @param aType the type of the curve to be queried.
 * @return True if the curve is plotted.
 */
bool DisplayCurveFit::hasCurve(const curveType &aType) {
  return m_plotPanel.at(aType)->hasCurve(m_curveTypeToQString.at(aType));
}

/**
 * @brief Creates a RangeSelector of selected type and plots it.
 * @param adcRange the type of dcRange to be displayed, either "fit" or
 * "evaluate".
 * @param aType the RangeSelector::SelectType
 * @post does nothing if the RangeSelector already exists
 */
void DisplayCurveFit::addRangeSelector(const dcRange &adcRange,
                                       RangeSelector::SelectType aType) {
  if (m_rangeSelector.find(adcRange) == m_rangeSelector.end()) {
    const QString &dcRangeName(m_dcRangeToQString.at(adcRange));
    m_rangeSelector.emplace(
        adcRange, m_uiForm.fitPlot->addRangeSelector(dcRangeName, aType));
    switch (adcRange) {
    case dcRange::fit:
      m_rangeSelector.at(adcRange)->setColour(QColor(Qt::black));
      break;
    case dcRange::evaluate:
      m_rangeSelector.at(adcRange)->setColour(QColor(Qt::red));
      break;
    }
  }
}

/**
 * @brief Display in the residuals panel the line at Y=0.
 */
void DisplayCurveFit::addResidualsZeroline() {
  if (m_uiForm.residualsPlot->hasRangeSelector(QString("zeroLine"))) {
    return; // do nothing
  }
  auto residualsZeroline = m_uiForm.residualsPlot->addRangeSelector(
      QString("zeroLine"), RangeSelector::YSINGLE);
  residualsZeroline->setColour(QColor(Qt::darkGreen));
  residualsZeroline->setMinimum(0.0);
}

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Private members +++++++
//               ++++++++++++++++++++++++++++++++

/**
 * @brief Find in member m_curveTypeToQString the curve type
 * corresponding to the query name.
 * @param name query name of the curve.
 * @return the curve type associated with the name.
 * @exception std::domain_error the name does not have a corresponding type.
 */
DisplayCurveFit::curveType
DisplayCurveFit::nameToType(const QString &name) const {
  auto foundPair =
      std::find_if(m_curveTypeToQString.begin(), m_curveTypeToQString.end(),
                   [&name](const std::pair<curveType, QString> &p) {
                     return p.second == name;
                   });
  if (foundPair == m_curveTypeToQString.end()) {
    throw std::domain_error("Curve name " + name.toStdString() +
                            " does not have a curveType");
  }
  return foundPair->first;
}

/**
 * @brief Translate a list of curve names to curve types.
 * @param curveNames query curve names.
 * @return a std::vector of the corresponding curve types.
 * @exception std::domain_error one (of more) of the names do not have a
 *correspoinding type.
 */
DisplayCurveFit::curveTypes
DisplayCurveFit::namesToTypes(const QStringList &curveNames) const {
  DisplayCurveFit::curveTypes typesFound;
  for (const auto &curveName : curveNames) {
    typesFound.push_back(this->nameToType(curveName));
  }
  return typesFound;
}

} // namespace MantidWidgets
} // namespace MantidQt
