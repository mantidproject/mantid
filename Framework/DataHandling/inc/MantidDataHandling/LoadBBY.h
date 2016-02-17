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

namespace Mantid {
namespace DataHandling {
/**
Loads a Bilby data file. Implements API::IFileLoader and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)
@date 11/07/2014

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

class DLLExport LoadBBY : public API::IFileLoader<Kernel::FileDescriptor> {

  struct InstrumentInfo {
    //
    int32_t bm_counts;
    int32_t att_pos;
    //
    double period_master;
    double period_slave;
    double phase_slave;
    //
    double L1_chopper_value;
    double L2_det_value;
    //
    double L2_curtainl_value;
    double L2_curtainr_value;
    double L2_curtainu_value;
    double L2_curtaind_value;
    //
    double D_det_value;
    //
    double D_curtainl_value;
    double D_curtainr_value;
    double D_curtainu_value;
    double D_curtaind_value;
  };

public:
  // construction
  LoadBBY() {}
  ~LoadBBY() override {}

  // description
  int version() const override { return 1; }
  const std::string name() const override { return "LoadBBY"; }
  virtual const std::string category() const { return "DataHandling"; }
  const std::string summary() const override {
    return "Loads a BilBy data file into a workspace.";
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
  static std::vector<bool> createRoiVector(const std::string &maskfile);

  // instrument creation
  Geometry::Instrument_sptr createInstrument(ANSTO::Tar::File &tarFile,
                                             InstrumentInfo &instrumentInfo);

  // load nx dataset
  template <class T>
  static bool loadNXDataSet(NeXus::NXEntry &entry, const std::string &path,
                            T &value);

  // binary file access
  template <class EventProcessor>
  static void loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor);
};

} // DataHandling
} // Mantid

#endif // DATAHANDING_LOADBBY_H_
