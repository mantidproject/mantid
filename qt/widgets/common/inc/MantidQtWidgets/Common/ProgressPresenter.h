// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ProgressBase.h"
#include "MantidQtWidgets/Common/ProgressableView.h"

class ProgressPresenter : public Mantid::Kernel::ProgressBase {
private:
  MantidQt::MantidWidgets::ProgressableView *const m_progressableView;

public:
  ProgressPresenter(double start, double end, int64_t nSteps,
                    MantidQt::MantidWidgets::ProgressableView *const progressableView)
      : ProgressBase(static_cast<int>(start), static_cast<int>(end), static_cast<int>(nSteps)),
        m_progressableView(progressableView) {
    if (!progressableView) {
      throw std::runtime_error("ProgressableView is null");
    }
    m_progressableView->clearProgress();
    m_progressableView->setProgressRange(static_cast<int>(start), static_cast<int>(end));
  }

  void doReport(const std::string & /*msg*/) override {
    if (m_progressableView->isPercentageIndicator())
      m_progressableView->setProgress(static_cast<int>(m_i));
  }
  void clear() { m_progressableView->clearProgress(); }
  void setAsPercentageIndicator() { m_progressableView->setAsPercentageIndicator(); }
  void setAsEndlessIndicator() { m_progressableView->setAsEndlessIndicator(); }
  virtual ~ProgressPresenter() = default;
};
