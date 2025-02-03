// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "LoadANSTOEventFile.h"
#include "LoadANSTOHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/LogManager.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

using ANSTO::EventVector_pt;

/*
Loads an ANSTO Pelican event file and stores it in an event workspace.

@author Geish Miladinovic (ANSTO)
*/

/** LoadPLN : Loads an ANSTO PLN Hdf and linked event file into a workspace.

Required Properties:
<UL>
<LI> Filename - Name of and path to the input event file</LI>
<LI> OutputWorkspace - Name of the workspace which stores the data</LI>
</UL>

Optional Properties:
<UL>
<LI> Mask - The input filename of the mask data</LI>
<LI> SelectDetectorTubes - Range of detector tubes to be loaded</LI>
<LI> FilterByTimeStart - Only include events after the start time</LI>
<LI> FilterByTimeStop - Only include events before the stop time</LI>
<LI> PathToBinaryEventFile - Rel or abs path to event file linked to hdf</LI>
<LI> SelectDataset - Select the linked event dataset</LI>
</UL>

*/
class MANTID_DATAHANDLING_DLL LoadPLN : public API::IFileLoader<Kernel::NexusHDF5Descriptor> {

public:
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;
  const std::string name() const override;
  const std::string summary() const override;
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

private:
  void exec() override;
  void init() override;
  void exec(const std::string &hdfFile, const std::string &eventFile);

  // region of intereset
  std::vector<bool> createRoiVector(const std::string &seltubes, const std::string &maskfile);

protected:
  // load parameters from input file
  void loadParameters(const std::string &hdfFile, API::LogManager &logm);
  void loadEnvironParameters(const std::string &hdfFile, API::LogManager &logm);

  // load the instrument definition and instrument parameters
  void loadInstrument();

  // get the L2 distance indexed by detector id
  void loadDetectorL2Values();

  // create workspace
  void createWorkspace(const std::string &title);

  // prepare event storage
  void prepareEventStorage(ANSTO::ProgressTracker &prog, std::vector<size_t> &eventCounts,
                           std::vector<EventVector_pt> &eventVectors);

  // set up the detector masks
  void setupDetectorMasks(const std::vector<bool> &roi);

  // shared member variables
  DataObjects::EventWorkspace_sptr m_localWorkspace;
  int32_t m_datasetIndex{0};
  std::string m_startRun;
  std::vector<double> m_detectorL2;
};

} // namespace DataHandling
} // namespace Mantid
