// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_ISAVEPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_ISAVEPRESENTER_H
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflBatchPresenter;

/** @class ISavePresenter

ISavePresenter is an interface which defines the functions that need
to be implemented by a concrete 'Save ASCII' tab presenter
*/
class ISavePresenter {
public:
  virtual ~ISavePresenter() = default;

  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;

  virtual void reductionCompletedForGroup(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) = 0;

  virtual void
  reductionCompletedForRow(MantidWidgets::DataProcessor::GroupData const &group,
                           std::string const &workspaceName) = 0;

  /// Tell the presenter something happened
  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_ISAVEPRESENTER_H */
