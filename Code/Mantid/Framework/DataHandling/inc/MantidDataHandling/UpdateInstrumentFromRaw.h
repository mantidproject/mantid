#ifndef MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMRAW_H_
#define MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMRAW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
	
  namespace Geometry
  {
    class CompAssembly;
    class Component;
    class Instrument;
  }
	
  namespace DataHandling
  {
    /** @class UpdateInstrumentFromRaw UpdateInstrumentFromRaw.h DataHandling/UpdateInstrumentFromRaw.h

    Updating detector positions initially loaded in from Instrument Defintion File (IDF) from information in raw files. 
    Note doing this will results in a slower performance (likely slightly slower performance) compared to specifying the 
    correct detector positions in the IDF in the first place. 

    Note that this algorithm moves the detectors without subsequent rotation, hence this means that detectors may not for 
    example face the sample perfectly after this algorithm has been applied.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace </LI>
    <LI> Filename - The name of and path to the input RAW file </LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 6/5/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */
    class DLLExport UpdateInstrumentFromRaw : public API::Algorithm
    {
    public:
      /// Default constructor
      UpdateInstrumentFromRaw();

      /// Destructor
      ~UpdateInstrumentFromRaw() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "UpdateInstrumentFromRaw";};

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();

      /// Overwrites Algorithm method. Does nothing at present
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMRAW_H_*/

