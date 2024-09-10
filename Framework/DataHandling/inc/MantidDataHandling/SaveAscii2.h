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
#include "MantidAPI/Axis.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveAscii2 SaveAscii2.h DataHandling/SaveAscii2.h

Saves a workspace or selected spectra in a coma-separated ascii file. Spectra
are saved in columns.

*/
namespace AxisHelper {

struct AxisProxy {
  AxisProxy(Mantid::API::Axis *ax) : a(ax) {}
  virtual double getCentre(const int index) { return a->getValue(index); }
  virtual ~AxisProxy() = default;

protected:
  Mantid::API::Axis *a;
};

struct BinEdgeAxisProxy : AxisProxy {
  using AxisProxy::AxisProxy;
  double getCentre(const int index) override { return (a->getValue(index) + a->getValue(index + 1)) / 2.; }
};

} // namespace AxisHelper

class MANTID_DATAHANDLING_DLL SaveAscii2 final : public API::Algorithm {
public:
  /// Default constructor
  SaveAscii2();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Saves a 2D workspace to a ascii file."; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadAscii", "SaveCSV", "SaveReflectometryAscii", "SaveOpenGenieAscii", "SaveGSS", "SaveFocusedXYE"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  void writeTableWorkspace(const API::ITableWorkspace_const_sptr &tws, const std::string &filename, bool appendToFile,
                           bool writeHeader, int prec, bool scientific, const std::string &comment);
  void write1DHistoCut(const API::IMDHistoWorkspace_const_sptr &mdws, const std::string &filename, bool appendToFile,
                       bool writeHeader, int prec, bool scientific, const std::string &comment);
  /// Writes a spectrum to the file using a workspace index
  void writeSpectrum(const int &wsIndex, std::ofstream &file);
  std::vector<std::string> stringListToVector(std::string &inputString);
  void populateQMetaData();
  void populateSpectrumNumberMetaData();
  void populateAngleMetaData();
  void populateAllMetaData();
  bool findElementInUnorderedStringVector(const std::vector<std::string> &vector, const std::string &toFind);
  /// Retrieve sample log value and its unit
  std::pair<std::string, std::string> sampleLogValueUnit(const std::string &logName);
  /// Write file header
  void writeFileHeader(const std::vector<std::string> &logList, std::ofstream &file);
  std::string createSpectrumFilename(size_t workspaceIndex);

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;

  int m_nBins;
  std::string m_sep;
  bool m_writeDX;
  bool m_writeID;
  bool m_isCommonBins;
  bool m_writeSpectrumAxisValue;
  API::MatrixWorkspace_const_sptr m_ws;
  std::vector<std::string> m_metaData;
  std::map<std::string, std::vector<std::string>> m_metaDataMap;
  std::unique_ptr<AxisHelper::AxisProxy> m_axisProxy;
  const std::vector<std::string> m_asciiExts = {".dat", ".txt", ".csv"};
};
} // namespace DataHandling
} // namespace Mantid
