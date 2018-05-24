#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_

#include "GSASIIRefineFitPeaksOutputProperties.h"
#include "GSASIIRefineFitPeaksParameters.h"

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
  void refinementsComplete();

  void refinementSuccessful(GSASIIRefineFitPeaksOutputProperties);

  void refinementFailed(std::string);

  void refinementCancelled();

private:
  EnggDiffGSASFittingModel *m_model;

  const std::vector<GSASIIRefineFitPeaksParameters> m_refinementParams;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFGSASFITTINGWORKER_H_
