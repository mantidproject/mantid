// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  Mantid::API::IAlgorithm_sptr alg;
  std::vector<GSASIIRefineFitPeaksOutputProperties> refinementResultSets;
  try {
    for (const auto params : m_refinementParams) {
      const auto fitResults = m_model->doGSASRefinementAlgorithm(params);
      alg = fitResults.first;
      refinementResultSets.emplace_back(fitResults.second);
      emit refinementSuccessful(alg, fitResults.second);
    }
    emit refinementsComplete(alg, refinementResultSets);
  } catch (const Mantid::API::Algorithm::CancelException &) {
    emit refinementCancelled();
  } catch (const std::exception &e) {
    emit refinementFailed(e.what());
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
