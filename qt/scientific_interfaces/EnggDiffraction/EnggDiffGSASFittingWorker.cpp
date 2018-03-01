#include "EnggDiffGSASFittingWorker.h"
#include "EnggDiffGSASFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingWorker::EnggDiffGSASFittingWorker(
    EnggDiffGSASFittingModel *model,
    const GSASIIRefineFitPeaksParameters &params)
    : m_model(model), m_refinementParams(params) {}

void EnggDiffGSASFittingWorker::doRefinement() {
  try {
    const auto outputProperties =
        m_model->doGSASRefinementAlgorithm(m_refinementParams);
    qRegisterMetaType<
        MantidQt::CustomInterfaces::GSASIIRefineFitPeaksOutputProperties>();
    emit refinementSuccessful(outputProperties);
  } catch (const std::exception &e) {
    emit refinementFailed(e.what());
  }
}

} // CustomInterfaces
} // MantidQt
