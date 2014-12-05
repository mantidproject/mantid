#ifndef DATAHANDING_LOADBBY_H_
#define DATAHANDING_LOADBBY_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidNexus/NexusClasses.h"
#include "LoadANSTOHelper.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a Bilby data file. Implements API::IFileLoader and its file check methods to
    recognise a file as the one containing Bilby data.

    @author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov (Tessella plc)
    @date 11/07/2014

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

    class DLLExport LoadBBY : public API::IFileLoader<Kernel::FileDescriptor> {
    public:
      // construction
      LoadBBY() {}
      virtual ~LoadBBY() {}
  
      // description 
      virtual int version() const { return 1; }
      virtual const std::string name() const { return "LoadBBY"; }
      virtual const std::string category() const { return "DataHandling"; }
      virtual const std::string summary() const {return "Loads a BilBy data file into an workspace."; }

      // returns a confidence value that this algorithm can load a specified file
      virtual int confidence(Kernel::FileDescriptor &descriptor) const;

    protected:
      // initialisation
      virtual void init();
      // execution
      virtual void exec();

    private:      
      // instrument creation
      Geometry::Instrument_sptr createInstrument(ANSTO::Tar::File &tarFile);

      // load nx dataset
      template<class T>
      static bool loadNXDataSet(T &value, NeXus::NXEntry &entry, const std::string &path);

      // binary file access
      template<class Counter>
      static void loadEvents(API::Progress &prog, const char *progMsg, ANSTO::Tar::File &file, const double tofMinBoundary, const double tofMaxBoundary, Counter &counter);
      static std::vector<bool> createMaskVector(const std::string &maskFilename, bool &maskFileLoaded);
    };
  }
}
#endif //DATAHANDING_LOADBBY_H_
