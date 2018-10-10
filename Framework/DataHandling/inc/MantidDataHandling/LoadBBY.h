// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DATAHANDING_LOADBBY_H_
#define DATAHANDING_LOADBBY_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "LoadANSTOHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/*
Loads a Bilby data file. Implements API::IFileLoader and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)
@date 11/07/2014
*/

class DLLExport LoadBBY : public API::IFileLoader<Kernel::FileDescriptor> {

  struct InstrumentInfo {
    //
    int32_t bm_counts;
    int32_t att_pos;
    bool is_tof;       // tof or wavelength data
    double wavelength; // -> /nvs067/lambda
    //
    std::string sample_name;
    std::string sample_description;
    double sample_aperture;
    double sample_x;
    double sample_y;
    double sample_z;
    //
    double source_aperture;
    int32_t master1_chopper_id;
    int32_t master2_chopper_id;
    //
    double period_master;
    double period_slave;
    double phase_slave;
    //
    double Lt0_value;
    double Ltof_curtainl_value;
    double Ltof_curtainr_value;
    double Ltof_curtainu_value;
    double Ltof_curtaind_value;
    //
    double L1_chopper_value;
    double L1_source_value;
    double L2_det_value;
    //
    double L2_curtainl_value;
    double L2_curtainr_value;
    double L2_curtainu_value;
    double L2_curtaind_value;
    //
    double D_curtainl_value;
    double D_curtainr_value;
    double D_curtainu_value;
    double D_curtaind_value;
  };

public:
  // description
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Load", "LoadQKK"};
  }
  const std::string name() const override { return "LoadBBY"; }
  const std::string category() const override { return "DataHandling\\ANSTO"; }
  const std::string summary() const override {
    return "Loads a Bilby data file into a workspace.";
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
  void createInstrument(ANSTO::Tar::File &tarFile,
                        InstrumentInfo &instrumentInfo);

  // load nx dataset
  template <class T>
  static bool loadNXDataSet(NeXus::NXEntry &entry, const std::string &path,
                            T &value);
  static bool loadNXString(NeXus::NXEntry &entry, const std::string &path,
                           std::string &value);

  // binary file access
  template <class EventProcessor>
  static void loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor);
};

} // namespace DataHandling
} // namespace Mantid

#endif // DATAHANDING_LOADBBY_H_
