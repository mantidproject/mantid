#ifndef MANTID_CUSTOMINTERFACES_PROGRESSPRESENTER_H
#define MANTID_CUSTOMINTERFACES_PROGRESSPRESENTER_H

#include "MantidKernel/ProgressBase.h"

class ProgressPresenter : public Mantid::Kernel::ProgressBase {
private:
  MantidQt::CustomInterfaces::ProgressableView *const m_progressableView;

public:
  ProgressPresenter(
      double start, double end, int64_t nSteps,
      MantidQt::CustomInterfaces::ProgressableView *const progressableView)
      : ProgressBase(static_cast<int>(start), static_cast<int>(end),
                     static_cast<int>(nSteps)),
        m_progressableView(progressableView) {
    if (!progressableView) {
      throw std::runtime_error("ProgressableView is null");
    }
    m_progressableView->clearProgress();
    m_progressableView->setProgressRange(static_cast<int>(start),
                                         static_cast<int>(end));
  }

  void doReport(const std::string &) {
    m_progressableView->setProgress(static_cast<int>(m_i));
  }
  void clear() { m_progressableView->clearProgress(); }
  ~ProgressPresenter() {}
};
#endif /* MANTID_CUSTOMINTERFACES_PROGRESSPRESENTER_H */
