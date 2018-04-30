#include "EnggDiffGSASFittingWorker.h"
#include "EnggDiffGSASFittingModel.h"

#include "MantidAPI/Algorithm.h"

Q_DECLARE_METATYPE(Mantid::API::IAlgorithm_sptr)

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
    qRegisterMetaType<Mantid::API::IAlgorithm_sptr>("IAlgorithm_sptr");
    for (const auto params : m_refinementParams) {
      const auto fitResults = m_model->doGSASRefinementAlgorithm(params);
      emit refinementSuccessful(fitResults.first, fitResults.second);
    }
    emit refinementsComplete();
  } catch (const Mantid::API::Algorithm::CancelException &) {
    emit refinementCancelled();
  } catch (const std::exception &e) {
    emit refinementFailed(e.what());
  }
}

} // CustomInterfaces
} // MantidQt
