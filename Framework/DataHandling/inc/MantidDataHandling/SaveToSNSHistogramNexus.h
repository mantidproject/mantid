// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D_fwd.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <climits>
// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

namespace Mantid {
namespace DataHandling {
/**
Save a Workspace2D or an EventWorkspace into a NeXus file whose format
corresponds to that expected at the SNS.
Uses an initial file to copy most of the contents, only with modified data and
time_of_flight fields.

@author Janik Zikovsky, with code from NXConvert, part of the NeXus library.
@date Dec 2, 2010
*/

class MANTID_DATAHANDLING_DLL SaveToSNSHistogramNexus final : public API::Algorithm {
public:
  /// Default constructor
  SaveToSNSHistogramNexus();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveToSNSHistogramNexus"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a workspace into SNS histogrammed NeXus format, using an "
           "original file as the starting point. This only works for "
           "instruments with Rectangular Detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of the output file
  std::string m_outputFilename;
  /// The name and path of the input file
  std::string m_inputFilename;
  /// Pointer to the local workspace
  API::MatrixWorkspace_const_sptr m_inputWorkspace;

  // Map from detector ID to WS index
  detid2index_map m_map;

  // Progress reporting
  std::unique_ptr<API::Progress> m_progress;

  bool m_compress;

  // Stuff needed by the copy_file() functions
  struct link_to_make {
    char from[1024]; /* path of directory with link */
    char name[256];  /* name of link */
    char to[1024];   /* path of real item */
  };

  struct link_to_make links_to_make[1024];
  int links_count;
  char current_path[1024];

  NXhandle inId, outId;

  int add_path(const char *path);
  int remove_path(const char *path);

  int WriteGroup(int is_definition);
  int WriteAttributes(int is_definition);
  int copy_file(const char *inFile, int nx_read_access, const char *outFile, int nx_write_access);

  int WriteOutDataOrErrors(const Geometry::RectangularDetector_const_sptr &det, int x_pixel_slab,
                           const char *field_name, const char *errors_field_name, bool doErrors, bool doBoth,
                           int is_definition, const std::string &bank);

  int WriteDataGroup(const std::string &bank, int is_definition);

  //
  //      // For iterating through the HDF file...
  //      void data(char *bank);
  //      herr_t attr_info(hid_t object_in_id, hid_t object_out_id);
  //      void time_of_flight(char *bank);
  //      herr_t file_info_bank(hid_t loc_id, const char *name, void *opdata);
  //      herr_t file_info_inst_bank(hid_t loc_id, const char *name, void
  //      *opdata);
  //      herr_t file_info_inst(hid_t loc_id, const char *name, void *opdata);
  //      herr_t file_info(hid_t loc_id, const char *name, void *opdata);
  //
  //      // Bunch of variables used by the HDF5 iterating functions
  //      hid_t       file_in_id, file_out_id;
  //      hid_t       grp_in_id, grp_out_id;
  //      hid_t       subgrp_in_id, subgrp_out_id;
  //      hid_t       subsubgrp_in_id, subsubgrp_out_id;
  //      hid_t       dataset_in_id, dataset_out_id;
  //      hid_t       dataspace;
  //      hid_t       filespace, memspace;      /* file and memory dataspace
  //      identifiers */
  //      herr_t      status;
  //      char cbank0[10],cbank[100];
  //      char ibank0[10],ibank[100];
};

} // namespace DataHandling
} // namespace Mantid
