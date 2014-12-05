#ifndef MANTID_DATAHANDLING_SAVECANSAS1D2_H
#define MANTID_DATAHANDLING_SAVECANSAS1D2_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "SaveCanSAS1D.h"
#include <fstream>  

namespace Poco{
  namespace XML{
    class Document;
    class Element;
    class Text;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SaveCanSAS1D2  DataHandling/SaveCanSAS1D2.h

    This algorithm saves  workspace into CanSAS1d format. This is an xml format except
    the \<Idata\>, \<\/Idata\> tags and all data in between must be one line, which necesitates
    the files be written iostream functions outside xml libraries.

    The second version of CanSAS1D implements the version 1.1, whose schema is found at
    http://www.cansas.org/formats/1.1/cansas1d.xsd. See the tutorial for more infomation 
    about: http://www.cansas.org/svn/1dwg/trunk/doc/cansas-1d-1_1-manual.pdf.
 
    The first version of SaveCanSAS1D implemented the version 1.0 of CanSAS.
    The main difference among them is the definition of the SASRoot and the 
    introduction of a new element called SAStransmission_spectrum, which allows
    to record the Spectrum of the transmission for the sample and can. 

    So, the SaveCanSAS1D2 will extend SaveCanSAS1D in the following: 
    
     - Introduction of 2 new (optional) workspace properties:
       - Transmission - The workspace for of the transmission
       - TransmissionCan - The workspace for the transmission can
     - Extension of the SaveCanSAS1D2::init method in order to introduce these workspaces properties.
     - Overide the SaveCanSAS1D2::createSASRootElement to conform the new header. 
     - Introduction of the method to deal with the new element: SaveCanSAS1D2::createSASTransElement. 
     - Override the SaveCanSAS1D2::exec method to introduce this new element when apropriated. 
     - Override the SaveCanSAS1D2::writeHeader method to introduce set the correct stylesheet
       
    @author Gesner Passos, Rutherford Appleton Laboratory
    @date 11/04/2013

    Copyright &copy; 2007-13 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport SaveCanSAS1D2 : public SaveCanSAS1D
    {
    public:
      /// default constructor
      SaveCanSAS1D2();
      virtual ~SaveCanSAS1D2();

      virtual int version() const { return 2; }

    protected:
      /// Extends the SaveCanSAS1D init method
      virtual void init();
      /// Overwrites Algorithm method
      virtual void exec();
      
      /// Create the SASRoot element
      virtual void createSASRootElement(std::string& rootElem);

      /// this method creates SAStransmission_spectrum element
      void createSASTransElement(std::string& sasTrans, const std::string & name  );
	  
      /// Overwrites writeHeader method
      virtual void writeHeader(const std::string & fileName);

      ///points to the workspace that will be written to file
      API::MatrixWorkspace_const_sptr m_trans_ws, m_transcan_ws;
    };
    
  }
}

#endif
