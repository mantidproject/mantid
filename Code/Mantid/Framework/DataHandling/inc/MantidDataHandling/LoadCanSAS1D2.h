#ifndef MANTID_DATAHANDLING_LoadCanSAS1D2_H
#define MANTID_DATAHANDLING_LoadCanSAS1D2_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "LoadCanSAS1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
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
    /** @class LoadCanSAS1D2  DataHandling/LoadCanSAS1D2.h

    This algorithm loads 1 CanSAS1d xml file into a workspace.
    It implements the CanSAS - version 1.1 standard 
    (http://www.cansas.org/svn/1dwg/tags/v1.1/cansas1d.xsd). 

    The main difference between the CanSAS1D version 1.0 (implemented at 
    version 1 of this algorithm (LoadCanSAS1D) and the version 1.1 is 
    that the later version introduced the element SAStransmission_spectrum. 
    
    This means that right now, a file may have the reduced data, as well as
    some spectra related to the transmission. In order not to break the 
    signature proposed on LoadCanSAS1D version 1.0, a new Property will be
    introduced: 
      - LoadTransmission - boolean flag with default False. 
    
    If the user let the LoadTransmissionData false, than the signature will be:
      - OutputWs = LoadCanSAS1D(filename)

    If the user set the LoadTransmission, than, it will receive the output in this
    way: 
      - OutputWs, TransWs, TransCanWs = LoadCanSAS1D(filename,LoadTransmission)
    The values of TransWs and TransCanWs may be None if the related data was not found
    at filename. 
       
    @author Gesner Passos, Rutherford Appleton Laboratory
    @date 12/04/2013

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LoadCanSAS1D2 : public LoadCanSAS1D
    {
    public:
      ///default constructor
      LoadCanSAS1D2();
      /// destructor
      virtual ~LoadCanSAS1D2();
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 2; }

    protected:
      /// Overwrites Algorithm method. Extend to create the LoadTransmission flag.
      virtual void init();

      virtual void exec(); 
      /// Extends the LoadEntry to deal with the possibility of Transmission data. 
      virtual API::MatrixWorkspace_sptr loadEntry(Poco::XML::Node * const workspaceData, std::string & runName);
      /// Add new method to deal with loading the transmission related data. 
      API::MatrixWorkspace_sptr loadTransEntry(Poco::XML::Node * const workspaceData, std::string & runName,
                                               std::string trans_name);

      std::vector<API::MatrixWorkspace_sptr> trans_gp, trans_can_gp;

    private:
      void processTransmission(std::vector<API::MatrixWorkspace_sptr>& trans_gp, const std::string & name, const std::string & output_name); 
    };
    
  }
}
#endif // MANTID_DATAHANDLING_LoadCanSAS1D2_H
