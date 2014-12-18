#ifndef MANTID_API_ICATLOG_H_
#define MANTID_API_ICATLOG_H_

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/CatalogSession.h"

namespace Mantid
{
  namespace ICat
  {
    class CatalogSearchParam;
  }

  namespace API
  {
    /**
     This class creates an interface for information catalogs to support multiple facilities

     @author Sofia Antony, ISIS Rutherford Appleton Laboratory
     @date 23/09/2010

     Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport ICatalog
    {
      public:
        /// Virtual destructor
        virtual ~ICatalog(){};
        /// method to login to a catalog
        virtual CatalogSession_sptr login(const std::string&,const std::string&,const std::string&,const std::string&)=0;
        /// logout from catalog
        virtual void logout()=0;
        ///Search investigations
        virtual void search(const ICat::CatalogSearchParam&,ITableWorkspace_sptr&, const int &,const int &)=0;
        /// Obtain the number of results returned by the search method.
        virtual int64_t getNumberOfSearchResults(const ICat::CatalogSearchParam&)=0;
        /// search logged in users data
        virtual void myData(ITableWorkspace_sptr &)=0;
        /// get datasets.
        virtual void getDataSets(const std::string& ,ITableWorkspace_sptr&)=0;
        /// get datafiles
        virtual void getDataFiles(const std::string&,ITableWorkspace_sptr &)=0;
        ///  instrument list
        virtual void listInstruments(std::vector<std::string>&)=0;
        /// get investigationtype lists
        virtual void listInvestigationTypes(std::vector<std::string>&)=0;
        /// keep alive
        virtual void keepAlive()=0;
    };

    typedef boost::shared_ptr<ICatalog> ICatalog_sptr;
    typedef boost::shared_ptr<const ICatalog> ICatalog_const_sptr;
  }
}

#endif /*MANTID_API_ICATLOG_H_*/
