// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_

#include "GSASIIRefineFitPeaksOutputProperties.h"
#include "GSASIIRefineFitPeaksParameters.h"

#include "MantidAPI/IAlgorithm_fwd.h"

#include <QObject>

/**
Worker for long-running tasks (ie GSASIIRefineFitPeaks) in the GSAS tab of the
Engineering Diffraction GUI. Emits finished() signal when refinement is complete
*/
namespace MantidQt {
namespace CustomInterfaces {

class EnggDiffGSASFittingModel;

class EnggDiffGSASFittingWorker : public QObject {
  Q_OBJECT

public:
  EnggDiffGSASFittingWorker(
      EnggDiffGSASFittingModel *model,
      const std::vector<GSASIIRefineFitPeaksParameters> &params);

public slots:
  void doRefinements();

signals:
  void refinementsComplete(
      Mantid::API::IAlgorithm_sptr /*_t1*/,
      std::vector<GSASIIRefineFitPeaksOutputProperties> /*_t2*/);

  void refinementSuccessful(Mantid::API::IAlgorithm_sptr /*_t1*/,
                            GSASIIRefineFitPeaksOutputProperties /*_t2*/);

  void refinementFailed(std::string /*_t1*/);

  void refinementCancelled();

private:
  EnggDiffGSASFittingModel *m_model;

  const std::vector<GSASIIRefineFitPeaksParameters> m_refinementParams;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_
