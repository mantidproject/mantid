// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameView.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL IOutputNamePresenter {
public:
  virtual ~IOutputNamePresenter() = default;

  virtual int findInsertIndexLabel(std::string const &basename) = 0;
  virtual std::string generateOutputLabel() = 0;
  virtual void generateLabelWarning() const = 0;
  virtual void handleUpdateOutputLabel() = 0;
  virtual void setWsSuffixes(std::vector<std::string> const &suffixes) = 0;

  virtual void setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix = "") = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputNamePresenter final : public IOutputNamePresenter {
public:
  OutputNamePresenter(IOutputNameView *view);
  ~OutputNamePresenter() override = default;

  int findInsertIndexLabel(std::string const &outputBasename) override;
  std::string generateOutputLabel() override;
  void generateLabelWarning() const override;
  void handleUpdateOutputLabel() override;

  void setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix = "") override;
  void setWsSuffixes(std::vector<std::string> const &suffixes) override;

private:
  IOutputNameView *m_view;
  std::vector<std::string> m_suffixes;
  std::string m_currBasename;
  std::string m_currOutputSuffix;
};

} // namespace CustomInterfaces
} // namespace MantidQt
