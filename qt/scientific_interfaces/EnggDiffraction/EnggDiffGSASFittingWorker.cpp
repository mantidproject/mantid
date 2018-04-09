#include "EnggDiffGSASFittingWorker.h"
#include "EnggDiffGSASFittingModel.h"

#include "MantidAPI/Algorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingWorker::EnggDiffGSASFittingWorker(
    EnggDiffGSASFittingModel *model,
    const std::vector<GSASIIRefineFitPeaksParameters> &params)
    : m_model(model), m_refinementParams(params) {}

void EnggDiffGSASFittingWorker::doRefinements() {
  try {
    qRegisterMetaType<
        MantidQt::CustomInterfaces::GSASIIRefineFitPeaksOutputProperties>(
        "GSASIIRefineFitPeaksOutputProperties");
    GSASIIRefineFitPeaksOutputProperties fitResults;
    for (const auto params : m_refinementParams) {
      fitResults = m_model->doGSASRefinementAlgorithm(params);
    }
    emit refinementSuccessful(fitResults);
  } catch (const Mantid::API::Algorithm::CancelException &) {
    emit refinementCancelled();
  } catch (const std::exception &e) {
    emit refinementFailed(e.what());
  }
}

} // CustomInterfaces
} // MantidQt
