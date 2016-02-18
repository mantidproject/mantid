#include "MantidQtMantidWidgets/DisplayCurveFit.h"
// includes for workspace handling

// includes for interface development

namespace {
Mantid::Kernel::Logger g_log("DisplayCurveFit");
}

namespace MantidQt {
namespace MantidWidgets {

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Public members ++++++++
//               ++++++++++++++++++++++++++++++++

std::map<DisplayCurveFit::curveType, QString> const DisplayCurveFit::m_curveTypeToQString{
    {DisplayCurveFit::curveType::data,  QString("data")},
    {DisplayCurveFit::curveType::guess, QString("guess")},
    {DisplayCurveFit::curveType::fit,   QString("fit")},
    {DisplayCurveFit::curveType::residuals, QString("residuals")}};

std::map<DisplayCurveFit::curveType, Qt::GlobalColor> const DisplayCurveFit::m_curveTypeToColor{
    {DisplayCurveFit::curveType::data,  Qt::black},
    {DisplayCurveFit::curveType::guess, Qt::blue},
    {DisplayCurveFit::curveType::fit,   Qt::red},
    {DisplayCurveFit::curveType::residuals, Qt::red}};

std::map<DisplayCurveFit::fitRange, QString> const DisplayCurveFit::m_fitRangeToQString{
    {DisplayCurveFit::fitRange::standard, QString("standard")},
    {DisplayCurveFit::fitRange::extended, QString("extended")}};

/// Constructor
DisplayCurveFit::DisplayCurveFit(QWidget *parent)
    : API::MantidWidget(parent){
  m_plotPanel = {{curveType::data, m_uiForm.fitPlot},
                  {curveType::guess, m_uiForm.fitPlot},
                  {curveType::fit, m_uiForm.fitPlot},
                  {curveType::residuals, m_uiForm.residualsPlot}};
}

DisplayCurveFit::~DisplayCurveFit(){

}

/**
 * Sets the range of the given axis scale to a given range in the two fitting
 * panels.
 *
 * @param range Pair of values for range
 * @param axisID ID of axis
 * @exception std::runtime_error if supplied range is nonsense
 */
void DisplayCurveFit::setAxisRange(QPair<double, double> range, int axisID) {
  m_uiForm.fitPlot->setAxisRange(range, axisID);
  m_uiForm.residualsPlot->setAxisRange(range, axisID);
}

/**
 * Finds which stored fitting curves are associated with the query workspace
 *
 * @param workspace Pointer to workspace
 * @return List of curve names
 */
DisplayCurveFit::curveTypes DisplayCurveFit::getCurvesForWorkspace(
    const Mantid::API::MatrixWorkspace_sptr workspace) {
  QStringList curveNames = m_uiForm.fitPlot->getCurvesForWorkspace(workspace);
  curveNames =
      curveNames + m_uiForm.residualsPlot->getCurvesForWorkspace(workspace);
  return this->namesToTypes(curveNames);
}

/**
 * Gets the X range of a curve given its curve type.
 *
 * @param atype the query curve type
 * @exception there is no associated curve in any of the two PreviewPlot  panels
 */
QPair<double, double> DisplayCurveFit::getCurveRange(const curveType &atype) {
  return m_plotPanel.at(atype)->getCurveRange(m_curveTypeToQString.at(atype));
}

/**
 * Gets the X range of a curve given a pointer to the workspace.
 *
 * @param Pointer to workspace
 * @retuns the range of the first curve associated to the workspace
 * @exception std::runtime_error no curves associated to the query workspace
 */
QPair<double, double> DisplayCurveFit::getCurveRange(
    const Mantid::API::MatrixWorkspace_sptr workspace) {
  curveTypes typesFound = this->getCurvesForWorkspace(workspace);
  if (typesFound.size() == 0) {
    throw std::runtime_error("No fitting curves associated to workspace" +
                             workspace->name());
  }
  return getCurveRange(typesFound[0]);
}

/**
 * Adds a workspace to the preview plot given a pointer to it.
 *
 * @param curveName Name of curve
 * @param workspace Pointer to the workspace
 * @param specIndex Spectrum index to plot
 */
void
DisplayCurveFit::addSpectrum(const curveType &aType,
                             const Mantid::API::MatrixWorkspace_sptr workspace,
                             const size_t specIndex) {
  std::cerr<<"Entered DisplayCurveFit::addSpectrum\n";
  const QString curveName{m_curveTypeToQString.at(aType)};
  std::cerr<<"curveName="<<curveName.toStdString()<<std::endl;
  const QColor curveColor(m_curveTypeToColor.at(aType));
  QString junk{curveColor.name()};
  std::cerr<<"curveColor="<<junk.toStdString()<<std::endl;
  std::cerr<<"m_plotPanel.at(aType)="<<m_plotPanel.at(aType)<<std::endl;
  m_plotPanel.at(aType)->addSpectrum(curveName, workspace, specIndex, curveColor);
}

/**
 * Removes curve of query type
 *
 * @param aType the curve type to remove
 */
void DisplayCurveFit::removeSpectrum(const curveType &aType) {
  m_plotPanel.at(aType)->removeSpectrum(m_curveTypeToQString.at(aType));
}

/**
 * Checks to see if a given curve type is present in any of the fitting panels
 *
 * @param curveName Curve name
 * @return True if curve is on plot
 */
bool DisplayCurveFit::hasCurve(const curveType &aType) {
  return m_plotPanel.at(aType)->hasCurve(m_curveTypeToQString.at(aType));
}

/**
 * Creates a RangeSelector of selected type, adds it to the plots and stores it.
 *
 * @param aRange fitRange type
 * @param type selects the RangeSelector::SelectType
 * @exception std::runtime_error when the range already exists
 * @return RangeSelector object
 */
RangeSelector *
DisplayCurveFit::addRangeSelector(const fitRange &aRange,
                                  RangeSelector::SelectType aType) {
  const QString rangeName(m_fitRangeToQString.at(aRange));
  m_uiForm.residualsPlot->addRangeSelector(rangeName, aType);
  return m_uiForm.fitPlot->addRangeSelector(rangeName, aType);
}

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Private members +++++++
//               ++++++++++++++++++++++++++++++++

/**
 * Search values of m_curveTypeToQString to find the key
 *
 * @param name name of the curve
 * @return the curve type associated with the nam
 * @exception std::domain_error the name does not have a corresponding type
 */
DisplayCurveFit::curveType DisplayCurveFit::nameToType(const QString &name) const {
  auto foundPair = std::find_if(m_curveTypeToQString.begin(),
                        m_curveTypeToQString.end(),
                        [&name](const std::pair<curveType,QString> &p){return p.second==name;} );
  if (foundPair == m_curveTypeToQString.end()){
      throw std::domain_error("Curve name "+name.toStdString()+" does not have a curveType");
  }
  return foundPair->first;
}

/**
 * Translate a list of curve names to curve types
 *
 * @param names curve names
 * @return a vector of the corresponding curve types
 * @exception std::domain_error one (of more) of the names do not have a
 *correspoinding type
 */
DisplayCurveFit::curveTypes DisplayCurveFit::namesToTypes(const QStringList &curveNames) const {
  DisplayCurveFit::curveTypes typesFound;
  for (auto it = curveNames.begin(); it != curveNames.end(); ++it) {
    typesFound.push_back(this->nameToType(*it));
  }
  return typesFound;
}

} // namespace MantidQt
} // namespace MantidWidgets
