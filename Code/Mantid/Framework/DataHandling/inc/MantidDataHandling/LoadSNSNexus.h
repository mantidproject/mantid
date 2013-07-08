#ifndef MANTID_DATAHANDLING_LOADSNSNEXUS_H_
#define MANTID_DATAHANDLING_LOADSNSNEXUS_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include <climits>

#include <boost/shared_array.hpp>


//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadSNSNexus LoadSNSNexus.h NeXus/LoadSNSNexus.h
    Loads a NeXus file that conforms to the TOFRaw instrument definition format and stores it in a 2D workspace.
    LoadTOFRawNeXus is an algorithm and as such inherits  from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadSNSNexus : public LoadTOFRawNexus, public API::DeprecatedAlgorithm
    {
    public:
      /// Default constructor
      LoadSNSNexus();
      /// Destructor
      ~LoadSNSNexus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const;

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const;
      /// category
      virtual const std::string category() const { return "Deprecated"; }

      /// Returns a confidence value that this algorithm can load a file (Required by base class)
      virtual int confidence(Kernel::HDFDescriptor &) const { return 0; }
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSNSNEXUS_H_*/
