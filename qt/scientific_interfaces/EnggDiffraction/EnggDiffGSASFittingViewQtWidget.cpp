#include "EnggDiffGSASFittingViewQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

void EnggDiffGSASFittingViewQtWidget::displayRwp(const double rwp) const {
  throw std::runtime_error("Not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getFocusedFileName() const {
  throw std::runtime_error("Not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getGSASIIProjectPath() const {
  throw std::runtime_error("Not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getInstrumentFileName() const {
  throw std::runtime_error("Not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getPathToGSASII() const {
  throw std::runtime_error("Not yet implemented");
}

double EnggDiffGSASFittingViewQtWidget::getPawleyDMin() const { 
  throw std::runtime_error("Not yet implemented");
}

double EnggDiffGSASFittingViewQtWidget::getPawleyNegativeWeight() const {
  throw std::runtime_error("Not yet implemented");
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getPhaseFileNames() const {
  throw std::runtime_error("Not yet implemented");
}

GSASRefinementMethod
EnggDiffGSASFittingViewQtWidget::getRefinementMethod() const {
  throw std::runtime_error("Not yet implemented");
}

std::pair<int, size_t>
EnggDiffGSASFittingViewQtWidget::getSelectedRunLabel() const {
  throw std::runtime_error("Not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::plotCurve(
    const std::vector<boost::shared_ptr<QwtData>> &curve) {
  throw std::runtime_error("Not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::resetCanvas() {
  throw std::runtime_error("Not yet implemented");
}

bool EnggDiffGSASFittingViewQtWidget::showRefinementResultsSelected() const {
  throw std::runtime_error("Not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::updateRunList(
    const std::vector<std::pair<int, size_t>> &runLabels) {
  throw std::runtime_error("Not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::userWarning(
    const std::string &warningDescription) const {
  throw std::runtime_error("Not yet implemented");
}

} // CustomInterfaces
} // MantidQt
