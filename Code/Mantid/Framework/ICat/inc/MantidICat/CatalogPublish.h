#ifndef MANTID_ICAT_CATALOGPUBLISH_H
#define MANTID_ICAT_CATALOGPUBLISH_H

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
 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_ICAT_DLL CatalogPublish : public API::Algorithm {
public:
  /// constructor
  CatalogPublish() : API::Algorithm() {}
  /// Destructor
  ~CatalogPublish() {}
  /// Algorithm's name for identification.
  virtual const std::string name() const { return "CatalogPublish"; }
  /// Summary of algorithms purpose.
  virtual const std::string summary() const {
    return "Allows the user to publish datafiles or workspaces to the "
           "information catalog.";
  }
  /// Algorithm's version for identification.
  virtual int version() const { return 1; }
  /// Algorithm's category for identification.
  virtual const std::string category() const { return "DataHandling\\Catalog"; }

private:
  /// Override algorithm initialisation method.
  void init();
  /// Override algorithm execute method.
  void exec();
  /// Stream the contents of a file to a given URL.
  void publish(std::istream &fileContents, const std::string &uploadURL);
  /// We want "SaveNexus" to take care of checking groups. Not this algorithm.
  bool checkGroups() { return false; }
  /// True if the extension of the file is a datafile.
  bool isDataFile(const std::string &filePath);
  /// Saves the workspace as a nexus file to the user's default directory.
  void saveWorkspaceToNexus(Mantid::API::Workspace_sptr &workspace);
  /// Publish the history of a given workspace.
  void publishWorkspaceHistory(
      Mantid::API::ICatalogInfoService_sptr &catalogInfoService,
      Mantid::API::Workspace_sptr &workspace);
  /// Generate the history of a given workspace.
  const std::string
  generateWorkspaceHistory(Mantid::API::Workspace_sptr &workspace);
};
}
}
#endif
