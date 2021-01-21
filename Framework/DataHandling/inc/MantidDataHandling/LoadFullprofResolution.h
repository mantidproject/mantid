// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
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

/** LoadFullprofResolution : Load Fullprof resolution (.irf) file to
TableWorkspace(s)
*/
class DLLExport LoadFullprofResolution : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadFullprofResolution"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadFullprofFile"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\DataHandling"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load Fullprof's resolution (.irf) file to one or multiple "
           "TableWorkspace(s) and/or where this is supported."
           " See description section, translate fullprof resolution fitting "
           "parameter into Mantid equivalent fitting parameters.";
  }

  /// Get row numbers of the parameters in the table workspace
  static void getTableRowNumbers(const API::ITableWorkspace_sptr &tablews, std::map<std::string, size_t> &parammap);

  /// Put parameters into a matrix workspace
  static void putParametersIntoWorkspace(const API::Column_const_sptr &, const API::MatrixWorkspace_sptr &ws, int nProf,
                                         std::string &parameterXMLString);

  /// Add an Ikeda-Carpenter PV ALFBE parameter
  static void addALFBEParameter(const API::Column_const_sptr &, Poco::XML::Document *mDoc, Poco::XML::Element *parent,
                                const std::string &paramName);

  /// Add set of Ikeda-Carpenter PV Sigma parameters
  static void addSigmaParameters(const API::Column_const_sptr &, Poco::XML::Document *mDoc, Poco::XML::Element *parent);

  /// Add set of Ikeda-Carpenter PV Gamma parameters
  static void addGammaParameters(const API::Column_const_sptr &, Poco::XML::Document *mDoc, Poco::XML::Element *parent);

  /// Add set of BackToBackExponential S parameters
  static void addBBX_S_Parameters(const API::Column_const_sptr &, Poco::XML::Document *mDoc,
                                  Poco::XML::Element *parent);

  /// Add set of BackToBackExponential A parameters
  static void addBBX_A_Parameters(const API::Column_const_sptr &, Poco::XML::Document *mDoc,
                                  Poco::XML::Element *parent);

  /// Add set of BackToBackExponential B parameters
  static void addBBX_B_Parameters(const API::Column_const_sptr &, Poco::XML::Document *mDoc,
                                  Poco::XML::Element *parent);

  /// Get value for XML eq attribute for parameter
  static std::string getXMLEqValue(const API::Column_const_sptr &, const std::string &name);

  /// Get value for XML eq attribute for squared parameter
  static std::string getXMLSquaredEqValue(const API::Column_const_sptr &column, const std::string &name);

  // Translate a parameter name from as it appears in the table workspace to its
  // name in the XML file
  static std::string getXMLParameterName(const std::string &name);

  /// Create Bank to Workspace Correspondence
  static void createBankToWorkspaceMap(const std::vector<int> &banks, const std::vector<int> &workspaces,
                                       std::map<int, size_t> &workspaceOfBank);

  /// Place to store the row numbers
  static std::map<std::string, size_t> m_rowNumbers;

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Load file to a vector of strings
  void loadFile(const std::string &filename, std::vector<std::string> &lines);

  /// Get the NPROF number
  int getProfNumber(const std::vector<std::string> &lines);

  /// Scan imported file for bank information
  void scanBanks(const std::vector<std::string> &lines, const bool useFileBankIDs, std::vector<int> &banks,
                 std::map<int, int> &bankstartindexmap, std::map<int, int> &bankendindexmap);

  /// Parse .irf file to a map
  void parseResolutionStrings(std::map<std::string, double> &parammap, const std::vector<std::string> &lines,
                              const bool useFileBankIDs, int bankid, int startlineindex, int endlineindex,
                              int profNumber);

  void parseBankLine(std::string line, double &cwl, int &bankid);

  /// Search token for profile number
  int searchProfile();

  /// Parse 1 bank of lines of profile 9
  void parseProfile9();

  /// Parse 1 bank of lines of profile 10
  void parseProfile10();

  /// Parse a value and prints warning if something is wrong
  double parseDoubleValue(const std::string &value, const std::string &label = std::string());

  /// Generate output workspace
  DataObjects::TableWorkspace_sptr genTableWorkspace(std::map<int, std::map<std::string, double>> bankparammap);

  ///// Generate bank information workspace
  // DataObjects::TableWorkspace_sptr
  // genInfoTableWorkspace(std::vector<int> banks);
  //                              const std::vector<int> &workspaces,
  //                              std::map<int, size_t> &WorkpsaceOfBank);
};

} // namespace DataHandling
} // namespace Mantid
