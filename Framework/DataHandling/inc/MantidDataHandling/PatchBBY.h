#ifndef DATAHANDING_PATCHBBY_H_
#define DATAHANDING_PATCHBBY_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidNexus/NexusClasses.h"
#include "LoadANSTOHelper.h"

namespace Mantid {
namespace DataHandling {
/**
Patches a Bilby data file. Implements API::Algorithm and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov (Tessella plc)
@date 22/01/2016

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport PatchBBY : public API::Algorithm {
public:
  // construction
  PatchBBY() {}
  virtual ~PatchBBY() {}

  // description
  virtual int version() const override {
    return 1;
  }
  virtual const std::string name() const override {
    return "PatchBBY";
  }
  virtual const std::string category() const override {
    return "DataHandling";
  }
  virtual const std::string summary() const override {
    return "Patches a BilBy data file.";
  }

protected:
  // initialisation
  virtual void init() override;
  // execution
  virtual void exec() override;
};

} // DataHandling
} // Mantid

#endif // DATAHANDING_PATCHBBY_H_