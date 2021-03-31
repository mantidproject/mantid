// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Kernel {
// forward Declaration
class InternetHelper;
} // namespace Kernel

namespace DataHandling {

/** DownloadFile : Downloads a file from a url to the file system
 */
class DLLExport DownloadFile : public API::Algorithm {
public:
  DownloadFile();
  ~DownloadFile() override;

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Load", "CatalogDownloadDataFiles"}; }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  Kernel::InternetHelper *m_internetHelper;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
