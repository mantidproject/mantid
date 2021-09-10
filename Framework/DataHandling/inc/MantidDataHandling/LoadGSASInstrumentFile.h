// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Poco {
namespace XML {
class Document;
class Element;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace DataHandling {

/** LoadGSASInstrumentFile : Load GSAS instrument file to TableWorkspace(s)
 */
class DLLExport LoadGSASInstrumentFile : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadGSASInstrumentFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load parameters from a GSAS Instrument file."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveGSASInstrumentFile", "LoadGSS", "FixGSASInstrumentFile"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\DataHandling"; }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Load file to a vector of strings
  void loadFile(const std::string &filename, std::vector<std::string> &lines);

  /// Get Histogram type
  std::string getHistogramType(const std::vector<std::string> &lines);

  /// Get Number of banks
  size_t getNumberOfBanks(const std::vector<std::string> &lines);

  /// Scan imported file for bank information
  void scanBanks(const std::vector<std::string> &lines, std::vector<size_t> &bankStartIndex);

  /// Parse bank in file to a map
  void parseBank(std::map<std::string, double> &parammap, const std::vector<std::string> &lines, size_t bankid,
                 size_t startlineindex);

  /// Find first INS line at or after lineIndex
  size_t findINSPRCFLine(const std::vector<std::string> &lines, size_t lineIndex, double &param1, double &param2,
                         double &param3, double &param4);

  /// Generate output workspace
  DataObjects::TableWorkspace_sptr genTableWorkspace(std::map<size_t, std::map<std::string, double>> bankparammap);
};

} // namespace DataHandling
} // namespace Mantid
