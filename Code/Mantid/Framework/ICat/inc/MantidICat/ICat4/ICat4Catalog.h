#ifndef MANTID_ICAT_ICAT4CATALOG_H_
#define MANTID_ICAT_ICAT4CATALOG_H_

#include "MantidAPI/ICatalog.h"
#include "MantidAPI/TableRow.h"
#include "MantidICat/CatalogSearchParam.h"

namespace Mantid
{
  namespace ICat
  {
    /**
    This class is responsible for the implementation of ICat4 version based information catalogs
    @author Jay Rainey, ISIS Rutherford Appleton Laboratory 
    @date 12/08/2013
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
    class  ICat4Catalog : public Mantid::API::ICatalog
    {
      public:
        /// Constructor
        ICat4Catalog();
        /// Destructor
        virtual ~ICat4Catalog();
        /// Log the user into the catalog system.
        virtual void login(const std::string& username,const std::string& password,const std::string& url);
        /// Log the user out of the catalog system.
        virtual void logout();
        /// Search the catalog for data.
        virtual void search(const CatalogSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& outputws);
        /// Show the logged in user's investigations search results.
        virtual void myData(Mantid::API::ITableWorkspace_sptr& outputws);
        /// Get datasets.
        virtual void getDataSets(const long long&investigationId,Mantid::API::ITableWorkspace_sptr& outputws);
        /// Get datafiles
        virtual void getDataFiles(const long long&investigationId,Mantid::API::ITableWorkspace_sptr& outputws);
        /// Get instruments list
        virtual void listInstruments(std::vector<std::string>& instruments);
        /// Get investigationtypes list
        virtual void listInvestigationTypes(std::vector<std::string>& invstTypes);
        /// Get file location strings
        virtual void getFileLocation(const long long&fileid,std::string& filelocation);
        /// Get urls
        virtual void getDownloadURL(const long long& fileid,std::string & fileLocation);
        /// Keep current session alive
        virtual void keepAlive();
        /// Keep alive in minutes
        virtual int keepAliveinminutes();

      private:
        // Ensures human friendly error messages are provided to the user.
        void throwErrorMessage(ICat4::ICATPortBindingProxy& icat);
        // Defines the SSL authentication scheme.
        void setSSLContext(ICat4::ICATPortBindingProxy& icat);
        // Saves "MyData" query result to output workspace.
        void saveInvestigations(std::vector<ICat4::xsd__anyType*> response, API::ITableWorkspace_sptr& outputws);
        // Creates a search query string based on inputs provided by the user.
        std::string getSearchQuery(const CatalogSearchParam& inputs);
        // Saves "DataFiles" result to output workspace.
        void saveDataFiles(std::vector<ICat4::xsd__anyType*> response, API::ITableWorkspace_sptr& outputws);
        // Helper method that formats a given timestamp.
        std::string formatDateTime(time_t timestamp);

        /**
         * Template method to save data to table workspace
         * @param input :: Pointer to input value.
         * @param table :: Table row reference.
         */
        template<class T>
        void savetoTableWorkspace(T* input,Mantid::API::TableRow &table)
        {
          if(input != 0)
          {
            table << *input;
          }
          else
          {
            table << "";
          }
        }
    };
  }
}

#endif
