#ifndef MANTID_ICAT_CATALOGSEARCH_H_
#define MANTID_ICAT_CATALOGSEARCH_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/DllConfig.h"

namespace Mantid
{
  namespace ICat
  {

    /**
      This class is responsible for searching the catalog using the properties specified.

      Required Properties:
      <UL>
        <LI> Investigation name - The name of the investigation to search </LI>
        <LI> Instrument - The instrument to use in the search </LI>
        <LI> Run range - The range of run numbers to search between </LI>
        <LI> Start date - The start date used for search </LI>
        <LI> End date - The end date used for search </LI>
        <LI> Keywords - The keywords used for search </LI>
        <LI> Investigation id - The id of an investigation to search for </LI>
        <LI> Investigators name - Search for all investigations this investigator is in </LI>
        <LI> Sample - The name of the sample used in an investigation <LI>
        <LI> Investigation abstract - The abstract of the investigation to be searched <LI>
        <LI> Investigation type - The type of the investigation to search for <LI>
        <LI> My data - Search through the investigations you are part of <LI>
      </UL>

      @author Sofia Antony, ISIS Rutherford Appleton Laboratory
      @date 04/11/2013
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
    class MANTID_ICAT_DLL CatalogSearch: public API::Algorithm
    {
    public:
      ///constructor
      CatalogSearch():API::Algorithm(){}
      ///destructor
      ~CatalogSearch() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CatalogSearch"; }
      ///Summary of algorithms purpose
      virtual const std::string summary() const { return "Searches all active catalogs using the provided input parameters."; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Catalog"; }

    private:
      /// Overwrites Algorithm init method.
      void init();
      /// Overwrites Algorithm exec method
      void exec();
      /// Get all inputs for the algorithm
      void getInputProperties(CatalogSearchParam& params);
      /// Parse the run-range input field, split it into start and end run, and set related parameters.
      void setRunRanges(std::string &runRange, CatalogSearchParam& params);
    };
  }
}

#endif
