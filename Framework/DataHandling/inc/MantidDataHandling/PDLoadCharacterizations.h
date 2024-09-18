// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

#include <iosfwd>

namespace Mantid {
namespace DataHandling {

/** LoadPDCharacterizations : Load a characterization file used in Powder
  Diffraction Reduction.
*/
class MANTID_DATAHANDLING_DLL PDLoadCharacterizations : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a characterization file used in Powder Diffraction Reduction.";
  }

private:
  void init() override;
  void exec() override;
  std::vector<std::string> getFilenames();
  int readFocusInfo(std::ifstream &file, const std::string &filename);
  void readCharInfo(std::ifstream &file, API::ITableWorkspace_sptr &wksp, const std::string &filename, int linenum);
  void readVersion0(const std::string &filename, API::ITableWorkspace_sptr &wksp);
  void readVersion1(const std::string &filename, API::ITableWorkspace_sptr &wksp);
  void readExpIni(const std::string &filename, API::ITableWorkspace_sptr &wksp);
};

} // namespace DataHandling
} // namespace Mantid
