// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameModel.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameView.h"

namespace MantidQt::CustomInterfaces {
class MANTID_SPECTROSCOPY_DLL IOutputNamePresenter {
public:
  virtual ~IOutputNamePresenter() = default;

  virtual std::string generateOutputLabel() = 0;
  virtual void generateWarningLabel() const = 0;
  virtual void handleUpdateOutputLabel() = 0;
  virtual std::string getCurrentLabel() = 0;
  virtual void hideOutputNameBox() const = 0;

  virtual void setWsSuffixes(std::vector<std::string> const &suffixes) = 0;
  virtual void setOutputWsBasename(std::string const &outputName, std::string const &outputSuffix = "") = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputNamePresenter final : public IOutputNamePresenter {
public:
  OutputNamePresenter(std::unique_ptr<IOutputNameModel> model, IOutputNameView *view);
  ~OutputNamePresenter() override = default;

  std::string generateOutputLabel() override;
  void generateWarningLabel() const override;
  void handleUpdateOutputLabel() override;
  std::string getCurrentLabel() override;
  void hideOutputNameBox() const override;

  void setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix = "") override;
  void setWsSuffixes(std::vector<std::string> const &suffixes) override;

private:
  std::unique_ptr<IOutputNameModel> m_model;
  IOutputNameView *m_view;
};
} // namespace MantidQt::CustomInterfaces
