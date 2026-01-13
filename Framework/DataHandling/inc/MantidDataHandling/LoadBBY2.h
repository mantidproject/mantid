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

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadANSTOHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidNexus/NexusClasses_fwd.h"
#include "MantidNexus/NexusDescriptorLazy.h"

namespace Mantid {
namespace DataHandling {
/*
Loads a Bilby data file. Implements API::IFileLoader and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)
@date 11/07/2014
*/

class MANTID_DATAHANDLING_DLL LoadBBY2 : public API::IFileLoader<Nexus::NexusDescriptorLazy> {

  struct InstrumentInfo {
    // core values or non standard conversion
    std::string sample_name;
    std::string sample_description;
    std::string start_time;
    int64_t bm_counts;
    int32_t att_pos;
    int64_t master1_chopper_id;
    int64_t master2_chopper_id;
    bool is_tof;       // tof or wavelength data
    double wavelength; // -> /nvs067/lambda
    double period_master;
    double period_slave;
    double phase_slave;
  };

public:
  // description
  LoadBBY2();
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Load", "LoadBBY"}; }
  const std::string name() const override { return "LoadBBY2"; }
  const std::string category() const override { return "DataHandling\\ANSTO"; }
  const std::string summary() const override { return "Loads a Bilby data file into a workspace."; }

  // returns a confidence value that this algorithm can load a specified file
  int confidence(Nexus::NexusDescriptorLazy &descriptor) const override;

protected:
  // initialisation
  void init() override;
  // execution
  void exec() override;

private:
  // region of interest
  static std::vector<bool> createRoiVector(const std::string &maskfile);

  // instrument creation
  void createInstrument(const Nexus::NXEntry &entry, uint64_t startTime, uint64_t endTime,
                        InstrumentInfo &instrumentInfo, std::map<std::string, double> &logParams,
                        std::map<std::string, std::string> &logStrings, std::map<std::string, std::string> &allParams);
  void loadInstrumentParameters(const Nexus::NXEntry &entry, uint64_t startTime, uint64_t endTime,
                                std::map<std::string, double> &logParams,
                                std::map<std::string, std::string> &logStrings,
                                std::map<std::string, std::string> &allParams);

  bool useHMScanTime{false};
};

} // namespace DataHandling
} // namespace Mantid
