// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace DataHandling {
/**
Loads a workspace from an ascii file. Spectra must be stored in columns.

Properties:
<ul>
<li>Filename  - the name of the file to read from.</li>
<li>Workspace - the workspace name that will be created and hold the loaded
data.</li>
<li>Separator - the column separation character: comma
(default),tab,space,colon,semi-colon.</li>
<li>Unit      - the unit to assign to the X axis (default: Energy).</li>
</ul>

@author Roman Tolchenov, Tessella plc
@date 3/07/09
*/
class MANTID_DATAHANDLING_DLL LoadSpec final : public API::Algorithm {
public:
  LoadSpec();
  const std::string name() const override { return "LoadSpec"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a text file and stores it in a 2D workspace "
           "(Workspace2D class).";
  }

  int version() const override { return 1; }
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  void init() override;
  void exec() override;

  /// Helper method for reading the number of spectra from the file
  size_t readNumberOfSpectra(std::ifstream &file) const;
  /// Helper method for reading a line from the file
  void readLine(const std::string &line, std::vector<double> &buffer) const;
  /// Helper method for reading a single histogram
  void readHistogram(const std::vector<double> &input, HistogramData::Histogram &histogram) const;

  /// Allowed values for the cache property
  std::vector<std::string> m_seperator_options;
  std::map<std::string, const char *> m_separatormap;          ///< a map of seperators
  using separator_pair = std::pair<std::string, const char *>; ///< serparator pair type def
};

} // namespace DataHandling
} // namespace Mantid
