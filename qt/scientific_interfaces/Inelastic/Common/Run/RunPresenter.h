// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt {
namespace CustomInterfaces {

class IRunSubscriber;
class IRunView;

class IRunPresenter {
public:
  virtual ~IRunPresenter() = default;

  virtual void handleRunClicked() = 0;

  virtual void setRunEnabled(bool const enable) = 0;
};

class RunPresenter final : public IRunPresenter {

public:
  RunPresenter(IRunSubscriber *subscriber, IRunView *view);

  void handleRunClicked() override;

  void setRunEnabled(bool const enable) override;

private:
  IRunSubscriber *m_subscriber;
  IRunView *m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt