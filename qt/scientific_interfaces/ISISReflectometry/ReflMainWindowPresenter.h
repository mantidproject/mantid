// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H

#include "DllConfig.h"
#include "IReflMainWindowPresenter.h"
#include "ReflBatchPresenterFactory.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowView;

/** @class ReflMainWindowPresenter

ReflMainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IReflMainWindowPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflMainWindowPresenter
    : public IReflMainWindowPresenter {
public:
  /// Constructor
  ReflMainWindowPresenter(IReflMainWindowView *view,
                          ReflBatchPresenterFactory batchPresenterFactory);
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;
  bool isProcessing() const override;
  void notifyHelpPressed() override;
  void notifyNewBatchRequested() override;
  void notifyCloseBatchRequested(int batchIndex) override;

private:
  void showHelp();
  IReflMainWindowView *m_view;
  ReflBatchPresenterFactory m_batchPresenterFactory;
  std::vector<std::shared_ptr<IReflBatchPresenter>> m_batchPresenters;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H */
