#ifndef MANTID_ICAT_CSEARCH_H_
#define MANTID_ICAT_CSEARCH_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include"MantidICat/ICatHelper.h"
#include "MantidICat/SearchParam.h"




namespace Mantid
{
	namespace ICat
	{
    
/** CSearch is a class responsible for SearchByRunNumber algorithm.
    This algorithm does the basic search and returns the investigations record

    Required Properties:
    <UL>
    <LI> Investigation name -The name of the investigation to search
	  <LI> Investigation Abstract - The abstract of the investigation to be searched
	  <LI> Sample - The name of the sample 
	  <LI> Investigators surname Investigators name
	  <LI> DataFile Name -The name of teh data file
    <LI> StartRun - The the start run number for search </LI>
    <LI> EndRun - The end run number for search </LI>
	  <LI> Rb Number - The RB number of the investigation
	  <LI> Investigation Type - The tye of the investigation
	  <LI> Instruments - The list of instruments used for search </LI>
	  <LI> StartDate - The start date used for search </LI>
	  <LI> EndDate - The end date used for search </LI>
	  <LI> Keywords - The keyswords used for search </LI>
	  <LI> Case Sensitive - The case ensitive boolean flag used for search </LI>
    </UL>
   
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory 
    @date 07/07/2010
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    
	*/	
		class DLLExport CSearch: public API::Algorithm
		{
		public:
			///constructor
			CSearch():API::Algorithm(){}
			///destructor
			~CSearch()
			{
			}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "CatalogSearch"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }
		private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
			/// Overwrites Algorithm init method.
			void init();
			/// Overwrites Algorithm exec method
			void exec();
			
			 /// get all inputs for the algorithm
			 void getInputProperties(CSearchParam& params);

											
		};
	}
}

#endif
