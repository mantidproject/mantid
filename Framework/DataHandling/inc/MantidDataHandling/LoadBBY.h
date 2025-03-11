// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "LoadANSTOHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"

namespace Mantid {
namespace DataHandling {
/*
Loads a Bilby data file. Implements API::IFileLoader and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)
@date 11/07/2014
*/

class MANTID_DATAHANDLING_DLL LoadBBY : public API::IFileLoader<Kernel::FileDescriptor> {

  struct InstrumentInfo {
    // core values or non standard conversion
    std::string sample_name;
    std::string sample_description;
    std::string start_time;
    int32_t bm_counts;
    int32_t att_pos;
    int32_t master1_chopper_id;
    int32_t master2_chopper_id;
    bool is_tof;       // tof or wavelength data
    double wavelength; // -> /nvs067/lambda
    double period_master;
    double period_slave;
    double phase_slave;
  };

public:
  // description
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Load", "LoadQKK"}; }
  const std::string name() const override { return "LoadBBY"; }
  const std::string category() const override { return "DataHandling\\ANSTO"; }
  const std::string summary() const override { return "Loads a Bilby data file into a workspace."; }

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
  void createInstrument(ANSTO::Tar::File &tarFile, InstrumentInfo &instrumentInfo,
                        std::map<std::string, double> &logParams, std::map<std::string, std::string> &logStrings,
                        std::map<std::string, std::string> &allParams);
  void loadInstrumentParameters(const NeXus::NXEntry &entry, std::map<std::string, double> &logParams,
                                std::map<std::string, std::string> &logStrings,
                                std::map<std::string, std::string> &allParams);

  // load nx dataset
  template <class T> static bool loadNXDataSet(const NeXus::NXEntry &entry, const std::string &path, T &value);
  bool loadNXString(const NeXus::NXEntry &entry, const std::string &path, std::string &value);

  // binary file access
  template <class EventProcessor>
  void loadEvents(API::Progress &prog, const char *progMsg, ANSTO::Tar::File &tarFile, EventProcessor &eventProcessor);
};

} // namespace DataHandling
} // namespace Mantid
