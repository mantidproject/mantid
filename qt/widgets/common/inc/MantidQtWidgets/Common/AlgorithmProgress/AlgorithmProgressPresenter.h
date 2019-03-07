// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSPRESENTER_H
#define ALGORITHMPROGRESSPRESENTER_H
#include "AlgorithmProgressModel.h"
#include "AlgorithmProgressPresenterBase.h"

namespace MantidQt {
namespace MantidWidgets {
class AlgorithmProgressWidget;

class AlgorithmProgressPresenter : public AlgorithmProgressPresenterBase {
  Q_OBJECT

public:
  AlgorithmProgressPresenter(AlgorithmProgressWidget *);

  void setCurrentAlgorithm() override;
  void updateProgressBar(Mantid::API::IAlgorithm_sptr, const double p,
                         const std::string &msg) override;

private:
  // The creator of the view also owns the view (Python), not this presenter.
  AlgorithmProgressWidget *view;
  AlgorithmProgressModel model;

  // The algorithm for which a progress bar is currently being controlled
  Mantid::API::IAlgorithm_sptr algorithm;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSPRESENTER_H