// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** LoadPDFgetNFile : TODO: DESCRIPTION
 */
class DLLExport LoadPDFgetNFile : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadPDFgetNFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Types of PDFgetN data files include .sqa, .sq, .gr, and etc."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadAscii"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

  /// Parse PDFgetN data file
  void parseDataFile(const std::string &filename);

  /// Check whether a string starts from a specified sub-string
  bool startsWith(const std::string &s, const std::string &header) const;

  /// Parse column name line staring with \#L
  void parseColumnNameLine(std::string line);

  /// Parse data line
  void parseDataLine(std::string line);

  /// Output data workspace
  DataObjects::Workspace2D_sptr outWS;

  /// Data structure to hold input:  Size = Number of columns in input file
  std::vector<std::vector<double>> mData;

  /// Names of the columns of the data
  std::vector<std::string> mColumnNames;

  /// Generate output workspace
  void generateDataWorkspace();

  /// Set X and Y axis unit and lebel
  void setUnit(const DataObjects::Workspace2D_sptr &ws);

  void checkSameSize(const std::vector<size_t> &numptsvec, size_t numsets);
};

} // namespace DataHandling
} // namespace Mantid
