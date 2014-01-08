#ifndef MANTID_ICAT_CATALOGPUBLISH_H
#define MATIND_ICAT_CATALOGPUBLISH_H

#include "MantidAPI/Algorithm.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

namespace Mantid
{
  namespace ICat
  {
    /**
     CatalogPublish is responsible for publishing user data to the data archive.

     @author Jay Rainey, ISIS Rutherford Appleton Laboratory
     @date 06/12/2013
     Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class CatalogPublish : public API::Algorithm
    {
      public:
        /// constructor
        CatalogPublish():API::Algorithm(){}
        /// Destructor
        ~CatalogPublish(){}
        /// Algorithm's name for identification.
        virtual const std::string name() const { return "CatalogPublish"; }
        /// Algorithm's version for identification.
        virtual int version() const { return 1; }
        /// Algorithm's category for identification.
        virtual const std::string category() const { return "DataHandling\\CatalogPublish"; }

      private:
        /// Sets documentation strings for this algorithm
        virtual void initDocs();
        /// Override algorithm initialisation method.
        void init();
        /// Override algorithm execute method.
        void exec();
        /// Stream the contents of a file to a given URL.
        void publish(std::istream& fileContents, const std::string &uploadURL);
        /// We want "SaveNexus" to take care of checking groups. Not this algorithm.
        bool checkGroups() { return false; }
        /// True if the extension of the file is a datafile.
        bool isDataFile(const std::string & filePath);
        /// Extracts the file name (e.g. CSP74683_ICPevent) from the file path.
        const std::string extractFileName(const std::string &filePath);
        /// Saves the workspace as a nexus file to the user's default directory.
        void saveWorkspaceToNexus(Mantid::API::Workspace_sptr &workspace);
        /// Publish the history of a given workspace.
        void publishWorkspaceHistory(Mantid::API::ICatalog_sptr &catalog, Mantid::API::Workspace_sptr &workspace, std::string &datafileName);
        /// Generate the history of a given workspace.
        const std::string generateWorkspaceHistory(Mantid::API::Workspace_sptr &workspace);
    };
  }
}
#endif
