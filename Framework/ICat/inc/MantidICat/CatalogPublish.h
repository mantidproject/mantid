// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 CatalogPublish is responsible for publishing user data to the data archive.

 Required Properties:

 <UL>
  <LI> InvestigationNumber - The number/id of the investigation in the archives
 to publish the data to.</LI>
  <LI> FileName - The path to the datafile to publish to the archives.</LI>
  <LI> InputWorkspace - The name of the workspace to publish to the
 archives.</LI>
 </UL>

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 06/12/2013
*/
class MANTID_ICAT_DLL CatalogPublish final : public API::Algorithm {
public:
  /// constructor
  CatalogPublish() : API::Algorithm() {}
  /// Destructor
  ~CatalogPublish() override = default;
  /// Algorithm's name for identification.
  const std::string name() const override { return "CatalogPublish"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Allows the user to publish datafiles or workspaces to the "
           "information catalog.";
  }
  /// Algorithm's version for identification.
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogLogin"}; }
  /// Algorithm's category for identification.
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  /// Override algorithm initialisation method.
  void init() override;
  /// Override algorithm execute method.
  void exec() override;
  /// Stream the contents of a file to a given URL.
  void publish(std::istream &fileContents, const std::string &uploadURL);
  /// We want "SaveNexus" to take care of checking groups. Not this algorithm.
  bool checkGroups() override { return false; }
  /// True if the extension of the file is a datafile.
  bool isDataFile(const std::string &filePath);
  /// Saves the workspace as a nexus file to the user's default directory.
  void saveWorkspaceToNexus(Mantid::API::Workspace_sptr &workspace);
  /// Publish the history of a given workspace.
  void publishWorkspaceHistory(Mantid::API::ICatalogInfoService_sptr &catalogInfoService,
                               Mantid::API::Workspace_sptr &workspace);
  /// Generate the history of a given workspace.
  const std::string generateWorkspaceHistory(Mantid::API::Workspace_sptr &workspace);
};
} // namespace ICat
} // namespace Mantid
