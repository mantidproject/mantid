// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

class IRunSubscriber;
class IRunView;

class MANTID_SPECTROSCOPY_DLL IRunPresenter {
public:
  virtual ~IRunPresenter() = default;

  virtual void handleRunClicked() = 0;

  virtual void setRunEnabled(bool const enable) = 0;
  virtual void setRunText(std::string const &text) = 0;

  virtual bool
  validate(std::unique_ptr<IUserInputValidator> validator = std::make_unique<UserInputValidator>()) const = 0;
};

class MANTID_SPECTROSCOPY_DLL RunPresenter final : public IRunPresenter {

public:
  RunPresenter(IRunSubscriber *subscriber, IRunView *view);

  void handleRunClicked() override;

  void setRunEnabled(bool const enable) override;
  void setRunText(std::string const &text) override;

  bool validate(std::unique_ptr<IUserInputValidator> validator = std::make_unique<UserInputValidator>()) const override;

private:
  IRunSubscriber *m_subscriber;
  IRunView *m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt
