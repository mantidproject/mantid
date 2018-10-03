#ifndef DATAHANDING_LOADEMU_H_
#define DATAHANDING_LOADEMU_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "LoadANSTOEventFile.h"
#include "LoadANSTOHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/LogManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/*
Loads an Emu data file. Implements API::IFileLoader and its file check methods
to recognise a file as the one containing Emu data.

Based on the LoadBBY implementation for which the original authors were
David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)

@author Geish Miladinovic (ANSTO)
@date 26/07/2018

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

class DLLExport LoadEMU : public API::IFileLoader<Kernel::FileDescriptor> {

  DataObjects::EventWorkspace_sptr m_localWorkspace;
  std::vector<double> m_detectorL2;

public:
  // description
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Load", "LoadQKK"};
  }
  const std::string name() const override { return "LoadEMU"; }
  const std::string category() const override { return "DataHandling\\ANSTO"; }
  const std::string summary() const override {
    return "Loads an EMU data file into a workspace.";
  }

  // returns a confidence value that this algorithm can load a specified file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

protected:
  // initialisation
  void init() override;
  // execution
  void exec() override;

private:
  // region of intreset
  static std::vector<bool> createRoiVector(const std::string &seltubes,
                                           const std::string &maskfile);

  // load parameters
  void loadParameters(ANSTO::Tar::File &tarFile, API::LogManager &logm);

  // create workspace
  void createWorkspace(ANSTO::Tar::File &tarFile);

  void loadDetectorL2Values();
  void updateNeutronicPostions(int detID, double sampleAnalyser);

  // binary file access
  template <class EventProcessor>
  static void loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor);
};

} // namespace DataHandling
} // namespace Mantid

#endif // DATAHANDING_LOADEMU_H_
