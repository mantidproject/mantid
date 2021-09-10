// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below the level
 *of
 *  DataHandling (e.g. Kernel, Geometry, API, DataObjects).
 *  I.e. It can only be used by plugin/algorithm-level packages (e.g.
 *DataHandling)
 *********************************************************************************/
#pragma once

#include "MantidDataObjects/Workspace2D.h"

class SANSInstrumentCreationHelper {
public:
  // Number of detector pixels in each dimension
  static const int nBins;
  // The test instrument has 2 monitors
  static const int nMonitors;

  /*
   * Generate a SANS test workspace, with instrument geometry.
   * The geometry is the SANSTEST geometry, with a 30x30 pixel 2D detector.
   *
   * @param workspace: name of the workspace to be created.
   */
  static Mantid::DataObjects::Workspace2D_sptr createSANSInstrumentWorkspace(const std::string &workspace);
  /** Run the Child Algorithm LoadInstrument (as for LoadRaw)
   * @param inst_name :: The name written in the Nexus file
   * @param workspace :: The workspace to insert the instrument into
   */
  static void runLoadInstrument(const std::string &inst_name, const Mantid::DataObjects::Workspace2D_sptr &workspace);

  /**
   * Populate spectra mapping to detector IDs
   *
   * @param workspace: Workspace2D object
   * @param nxbins: number of bins in X
   * @param nybins: number of bins in Y
   */
  static void runLoadMappingTable(const Mantid::DataObjects::Workspace2D_sptr &workspace, int nxbins, int nybins);
};
