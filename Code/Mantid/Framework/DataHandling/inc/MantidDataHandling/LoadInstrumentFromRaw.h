#ifndef MANTID_DATAHANDLING_LOADINSTRUMENTFROMRAW_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENTFROMRAW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace Geometry {
class CompAssembly;
class Component;
class Instrument;
}

namespace DataHandling {
/** @class LoadInstrumentFromRaw LoadInstrumentFromRaw.h
DataHandling/LoadInstrumentFromRaw.h

Attempt to load information about the instrument from a ISIS raw file. In
particular attempt to
read L2 and 2-theta detector position values and add detectors which are
positioned relative
to the sample in spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also
adds dummy source
and samplepos components to instrument.

LoadInstrumentFromRaw is intended to be used as a child algorithm of
other Loadxxx algorithms, rather than being used directly.
LoadInstrumentFromRaw is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> Workspace - The name of the workspace in which to use as a basis for any
data to be added.</LI>
<LI> Filename - The name of and path to the input RAW file </LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 2/5/2008

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
*/
class DLLExport LoadInstrumentFromRaw : public API::Algorithm {
public:
  /// Default constructor
  LoadInstrumentFromRaw();

  /// Destructor
  ~LoadInstrumentFromRaw() {}

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadInstrumentFromRaw"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Attempts to load information about the instrument from a ISIS raw "
           "file. In particular attempt to read L2 and 2-theta detector "
           "position values and add detectors which are positioned relative to "
           "the sample in spherical coordinates as "
           "(r,theta,phi)=(L2,2-theta,0.0). Also adds dummy source and "
           "samplepos components to instrument.  If the L1 source - sample "
           "distance is not available in the file then it may be read from the "
           "mantid properties file using the key instrument.L1, as a final "
           "fallback a default distance of 10m will be used.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Instrument;DataHandling\\Raw";
  }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init();

  /// Overwrites Algorithm method
  void exec();

  /// The name and path of the input file
  std::string m_filename;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENTFROMRAW_H_*/
