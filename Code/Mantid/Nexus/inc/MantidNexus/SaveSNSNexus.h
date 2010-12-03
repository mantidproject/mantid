#ifndef MANTID_NEXUS_SAVESNSNEXUS_H_
#define MANTID_NEXUS_SAVESNSNEXUS_H_

//Copy of the NexusCpp API was placed in MantidNexus
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"

using namespace ::NeXus;


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <climits>

namespace Mantid
{
  namespace NeXus
  {
    /**
    Save a Workspace2D into a Nexus file whose format corresponds to that expected at the SNS.
    Uses an initial file to copy most of the contents, only with modified data.

    @author Janik Zikovsky, with code from NXConvert, part of the NeXus library.
    @date Dec 2, 2010

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

    class DLLExport SaveSNSNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveSNSNexus();

      /// Destructor
      ~SaveSNSNexus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveSNSNexus";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling";}

    private:

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the output file
      std::string m_outputFilename;
      /// The name and path of the input file
      std::string m_inputFilename;
      /// Pointer to the local workspace
      API::MatrixWorkspace_const_sptr inputWorkspace;
      // Input workspace name.
      std::string m_inputWorkspaceName;

      // Map from detector ID to WS index
      API::IndexToIndexMap * map;


      struct link_to_make
      {
          char from[1024];   /* path of directory with link */
          char name[256];    /* name of link */
          char to[1024];     /* path of real item */
      };

      struct link_to_make links_to_make[1024];
      int links_count;
      char current_path[1024];

      NXhandle inId, outId;

      int add_path(const char* path);
      int remove_path(const char* path);

      int WriteGroup (int is_definition);
      int WriteAttributes (int is_definition);
      int convert_file(const char* inFile, int nx_read_access, const char* outFile, int nx_write_access);

//
//      // For iterating through the HDF file...
//      void data(char *bank);
//      herr_t attr_info(hid_t object_in_id, hid_t object_out_id);
//      void time_of_flight(char *bank);
//      herr_t file_info_bank(hid_t loc_id, const char *name, void *opdata);
//      herr_t file_info_inst_bank(hid_t loc_id, const char *name, void *opdata);
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
//      hid_t       filespace, memspace;      /* file and memory dataspace identifiers */
//      herr_t      status;
//      char cbank0[10],cbank[100];
//      char ibank0[10],ibank[100];
//      int mpi_size, mpi_rank;

    };

  } // namespace NeXus
} // namespace Mantid

#endif
