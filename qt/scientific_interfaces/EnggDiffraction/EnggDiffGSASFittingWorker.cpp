#include "EnggDiffGSASFittingWorker.h"
#include "EnggDiffGSASFittingModel.h"

#include "../../../Framework/PythonInterface/inc/MantidPythonInterface/kernel/Environment/ErrorHandling.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingWorker::EnggDiffGSASFittingWorker(
    EnggDiffGSASFittingModel *model,
    const GSASIIRefineFitPeaksParameters &params)
    : m_model(model), m_refinementParams(params) {}

void EnggDiffGSASFittingWorker::doRefinement() {
  try {
    qRegisterMetaType<
        MantidQt::CustomInterfaces::GSASIIRefineFitPeaksOutputProperties>(
        "GSASIIRefineFitPeaksOutputProperties");
    const auto outputProperties =
        m_model->doGSASRefinementAlgorithm(m_refinementParams);
    emit refinementSuccessful(outputProperties);
  } catch (const Mantid::PythonInterface::Environment::PythonException &e) {
    emit refinementCancelled();
  } catch (const std::exception &e) {
    emit refinementFailed(e.what());
  }
}

} // CustomInterfaces
} // MantidQt
