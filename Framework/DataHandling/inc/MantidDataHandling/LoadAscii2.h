// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/FileDescriptor.h"

#include <list>

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

@author Keith Brown, ISIS, Placement student from the University of Derby
@date 10/10/13
*/
class MANTID_DATAHANDLING_DLL LoadAscii2 : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Default constructor
  LoadAscii2();
  /// The name of the algorithm
  const std::string name() const override { return "LoadAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a text file and stores it in a 2D workspace "
           "or Table Workspace.";
  }

  /// The version number
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"SaveAscii"}; }
  /// The category
  const std::string category() const override { return "DataHandling\\Text"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

protected:
  /// Read the data from the file
  virtual API::Workspace_sptr readData(std::ifstream &file);
  /// Read the data from the file into a table workspace
  virtual API::Workspace_sptr readTable(std::ifstream &file);
  /// Return true if the line is to be skipped
  bool skipLine(const std::string &line, bool header = false) const;
  /// Return true if the line doesn't start with a valid character
  bool badLine(const std::string &line) const;
  /// check and configure flags and values relating to starting a new spectra
  void newSpectra();
  /// Check if the file has been found to incosistantly include spectra IDs
  void inconsistantIDCheck() const;
  /// Split the data into columns.
  int splitIntoColumns(std::list<std::string> &columns, const std::string &str) const;
  /// Fill the given vector with the data values
  void fillInputValues(std::vector<double> &values, const std::list<std::string> &columns) const;
  // write the values in the current line to teh end fo teh current spectra
  void addToCurrentSpectra(const std::list<std::string> &columns);
  // check that the number of columns in the current line match the number found
  // previously
  void checkLineColumns(const size_t &cols) const;
  // interpret a line that has been deemed valid enough to look at.
  void parseLine(const std::string &line, std::list<std::string> &columns);
  // find the number of columns we should expect from now on
  void setcolumns(std::ifstream &file, std::string &line, std::list<std::string> &columns);
  // wirte the spectra to the workspace
  void writeToWorkspace(API::MatrixWorkspace_sptr &localWorkspace, const size_t &numSpectra) const;
  // Process the header information. This implementation just skips it entirely.
  void processHeader(std::ifstream &file);
  // Set the Distribution on the workspace, either from input property or file header
  bool setDistribution(std::ifstream &file);
  /// The column separator
  std::string m_columnSep;

private:
  /// Declare properties
  void init() override;
  /// Execute the algorithm
  void exec() override;

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;
  std::string m_comment;
  size_t m_baseCols;
  size_t m_specNo;
  size_t m_lastBins;
  size_t m_curBins;
  bool m_spectraStart;
  size_t m_spectrumIDcount;
  size_t m_lineNo;
  std::vector<DataObjects::Histogram1D> m_spectra;
  std::unique_ptr<DataObjects::Histogram1D> m_curSpectra;
  std::vector<double> m_curDx;
  std::vector<double> m_spectrumAxis;
  std::vector<double> m_curHistoX;
  std::vector<double> m_curHistoY;
  std::vector<double> m_curHistoE;
};

} // namespace DataHandling
} // namespace Mantid
