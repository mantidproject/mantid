#ifndef MANTID_ICAT_CATALOGMYDATASEARCH_H_
#define MANTID_ICAT_CATALOGMYDATASEARCH_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidICat/DllConfig.h"

namespace Mantid
{
  namespace ICat
  {

    /**
    This algorithm obtains all of the information for the investigations the logged in user is an investigator of.

    Required Properties:
    <UL>
     <LI>  OutputWorkspace - name of the OutputWorkspace which contains my investigations search
    </UL>

    @author Sofia Antony, ISIS Rutherford Appleton Laboratory 
    @date 04/08/2010
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
    class MANTID_ICAT_DLL CatalogMyDataSearch: public API::Algorithm
    {
    public:
      ///constructor
      CatalogMyDataSearch():API::Algorithm(){}
      ///destructor
      ~CatalogMyDataSearch() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CatalogMyDataSearch"; }
      /// Summary of algorithms purpose.
      virtual const std::string summary() const { return "Obtains the user's investigations for all active catalogs and stores them into a workspace."; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Catalog"; }

    private:
      /// Overwrites Algorithm init method.
      void init();
      /// Overwrites Algorithm exec method
      void exec();
    };
  }
}

#endif
