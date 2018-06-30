#ifndef DATAHANDING_LOADQKK_H_
#define DATAHANDING_LOADQKK_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {
/**
     Loads a Quokka data file. Implements API::IFileLoader and its file check
   methods to
     recognise a file as the one containing QUOKKA data.

     @author Roman Tolchenov, Tessella plc
     @date 31/10/2011

     Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
     Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport LoadQKK : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadQKK"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a ANSTO QKK file. ";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadBBY"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
#endif // DATAHANDING_LOADQKK_H_
