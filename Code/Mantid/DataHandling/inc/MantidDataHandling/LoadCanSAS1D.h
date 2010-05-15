#ifndef MANTID_DATAHANDLING_LoadCanSAS1D_H
#define MANTID_DATAHANDLING_LoadCanSAS1D_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
//----------------------------------------------------------------------

namespace Poco {
  namespace XML {
    class Element;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadCanSAS1D  DataHandling/LoadCanSAS1D.h

    This algorithm loads 1 CanSAS1d xml file into a workspace.

    Required properties:
    <UL>
    <LI> OutputWorkspace - The name of workspace to be created.</LI>
    <LI> Filename - Name of the file to load</LI>
    </UL>
       
    @author Sofia Antony, Rutherford Appleton Laboratory
    @date 26/01/2010

    Copyright &copy; 2007-10 STFC Rutherford Appleton Laboratory

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
    class DLLExport LoadCanSAS1D : public API::Algorithm
    {
    public:
      ///default constructor
      LoadCanSAS1D();
      /// destructor
      ~LoadCanSAS1D();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadCanSAS1D"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      /// This method throws not found error if a element is not found in the xml file
      void throwException(Poco::XML::Element* elem,const std::string & name,const std::string& fileName);
      /// Run LoadInstrument sub algorithm
      void runLoadInstrument(const std::string & inst_name,DataObjects::Workspace2D_sptr localWorkspace);


    };
    
  }
}

#endif
