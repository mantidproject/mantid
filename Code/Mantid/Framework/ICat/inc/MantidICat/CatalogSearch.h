#ifndef MANTID_ICAT_CSEARCH_H_
#define MANTID_ICAT_CSEARCH_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidICat/CatalogSearchParam.h"

namespace Mantid
{
  namespace ICat
  {

    /**
      CatalogSearch is a class responsible for SearchByRunNumber algorithm.
      This algorithm does the basic search and returns the investigations record

      Required Properties:
      <UL>
        <LI> Investigation name - The name of the investigation to search </LI>
        <LI> Instrument - The instrument to use in the search </LI>
        <LI> Run range - The range of run numbers to search between </LI>
        <LI> StartDate - The start date used for search </LI>
        <LI> EndDate - The end date used for search </LI>
        <LI> Keywords - The keywords used for search </LI>
        <LI> Investigators name - Search for all investigations this investigator is in </LI>
        <LI> Sample - The name of the sample used in an investigation <LI>
        <LI> Investigation Abstract - The abstract of the investigation to be searched <LI>
        <LI> Investigation Type - The type of the investigation to search for <LI>
        <LI> My data - Search through the investigations you are part of <LI>
      </UL>

      @author Sofia Antony, ISIS Rutherford Appleton Laboratory
      @date 04/11/2013
      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport CatalogSearch: public API::Algorithm
    {
    public:
      ///constructor
      CatalogSearch():API::Algorithm(){}
      ///destructor
      ~CatalogSearch() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CatalogSearch"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Catalog"; }

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
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
