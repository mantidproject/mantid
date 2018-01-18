#include "EnggDiffGSASFittingViewQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

void MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::displayRwp(const double rwp) const{
    }

std::string MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getFocusedFileName() const{
      return std::string();
    }

std::string MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getGSASIIProjectPath() const{
      return std::string();
    }

std::string MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getInstrumentFileName() const {
      return std::string();
    }

std::string MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getPathToGSASII() const{
      return std::string();
    }

double MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getPawleyDMin() const {
      return 0.0;
    }

    double MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getPawleyNegativeWeight() const
    {
      return 0.0;
    }

    std::vector<std::string> MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getPhaseFileNames() const
    {
      return std::vector<std::string>();
    }

    GSASRefinementMethod MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getRefinementMethod() const
    {
      return GSASRefinementMethod();
    }

    std::pair<int, size_t> MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::getSelectedRunLabel() const
    {
      return std::pair<int, size_t>();
    }

    void MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::plotCurve(const std::vector<boost::shared_ptr<QwtData>>& curve)
    {
    }

    void MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::resetCanvas()
    {
    }

    bool MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::showRefinementResultsSelected() const
    {
      return false;
    }

    void MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::updateRunList(const std::vector<std::pair<int, size_t>>& runLabels)
    {
    }

    void MantidQt::CustomInterfaces::EnggDiffGSASFittingViewQtWidget::userWarning(const std::string & warningDescription) const
    {
    }

  } // CustomInterfaces
} // MantidQt
