// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include <string>
#include <vector>
namespace MantidQt::CustomInterfaces {

class MANTID_SPECTROSCOPY_DLL IOutputNameModel {
public:
  virtual ~IOutputNameModel() = default;

  virtual int findIndexToInsertLabel(std::string const &basename) = 0;

  virtual void setSuffixes(std::vector<std::string> const &suffixes) = 0;
  virtual void setOutputSuffix(std::string const &outputSuffix) = 0;
  virtual void setOutputBasename(std::string const &outputBasename) = 0;

  virtual std::vector<std::string> suffixes() const = 0;
  virtual std::string outputSuffix() const = 0;
  virtual std::string outputBasename() const = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputNameModel final : public IOutputNameModel {
public:
  OutputNameModel();
  ~OutputNameModel() override = default;

  int findIndexToInsertLabel(std::string const &outputBasename) override;

  void setSuffixes(std::vector<std::string> const &suffixes) override;
  void setOutputSuffix(std::string const &outputSuffix) override;
  void setOutputBasename(std::string const &outputBasename) override;

  std::vector<std::string> suffixes() const override;
  std::string outputSuffix() const override;
  std::string outputBasename() const override;

private:
  std::vector<std::string> m_suffixes;
  std::string m_currBasename;
  std::string m_currOutputSuffix;
};
} // namespace MantidQt::CustomInterfaces
