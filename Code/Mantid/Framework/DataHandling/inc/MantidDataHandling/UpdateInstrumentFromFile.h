#ifndef MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMFILE_H_
#define MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Geometry
  {
    class Instrument;
  }
	
  namespace DataHandling
  {
    /**

    Update detector positions initially loaded in from Instrument Defintion File (IDF) from information in the provided files. 
    Note doing this will result in a slower performance (likely slightly slower performance) compared to specifying the 
    correct detector positions in the IDF in the first place. 

    Note that this algorithm moves the detectors without subsequent rotation, hence this means that detectors may not for 
    example face the sample perfectly after this algorithm has been applied.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace </LI>
    <LI> Filename - The name of and path to the input RAW file </LI>
    </UL>

    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport UpdateInstrumentFromFile : public API::Algorithm
    {
    public:
      /// Default constructor
      UpdateInstrumentFromFile();

      /// Destructor
      ~UpdateInstrumentFromFile() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "UpdateInstrumentFromFile"; }
      /// Algorithm's alias for the old UpdateInstrumentFromRaw
      virtual const std::string alias() const { return "UpdateInstrumentFromRaw"; }

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

      /// Assumes the file is a raw file
      void updateFromRaw(boost::shared_ptr<Geometry::Instrument> instrument, 
                         const std::string & filename);
      /// Assumes the file is an ISIS NeXus file
      void updateFromIsisNeXus(boost::shared_ptr<Geometry::Instrument> instrument, 
                               const std::string & filename);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_UPDATEINSTRUMENTFROMFILE_H_*/

