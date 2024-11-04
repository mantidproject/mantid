// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 CatalogDownloadDataFiles is responsible for downloading datafiles from a
 catalog.

 Required Properties:

 <UL>
  <LI> Filenames - List of files to download </LI>
  <LI> InputWorkspace - The name of the workspace whioch stored the last
 investigation search results </LI>
  <LI> FileLocations - List of files with location which is downloaded </LI>
 </UL>

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 07/07/2010
*/
class MANTID_ICAT_DLL CatalogDownloadDataFiles final : public API::Algorithm {
public:
  /// Constructor
  CatalogDownloadDataFiles() : API::Algorithm(), m_prog(0.0) {}
  /// Destructor
  ~CatalogDownloadDataFiles() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogDownloadDataFiles"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Downloads datafiles from the archives based on the ID of a "
           "datafile.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"DownloadFile", "CatalogGetDataFiles", "CatalogLogin"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Catalog"; }

  /** This method is used for unit testing purpose.
   * as the Poco::Net library httpget throws an exception when the nd server n/w
   * is slow
   * I'm testing the download from mantid server.
   * as the download method I've written is private I can't access that in unit
   * testing.
   * so adding this public method to call the private download method and
   * testing.
   */
  std::string testDownload(const std::string &URL, const std::string &fileName);

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// True if the extension of the file is a datafile.
  bool isDataFile(const std::string &fileName);
  /// Saves the downloaded file to disc
  std::string saveFiletoDisk(std::istream &rs, const std::string &fileName);
  /// Saves downloaded file to local disk
  std::string doDownloadandSavetoLocalDrive(const std::string &URL, const std::string &fileName);

private:
  /// progress indicator
  double m_prog;
};
} // namespace ICat
} // namespace Mantid
