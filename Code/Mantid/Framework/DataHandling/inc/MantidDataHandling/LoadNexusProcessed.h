#ifndef MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"

#include "MantidNexus/NexusClasses.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid
{

  namespace DataHandling
  {
    /**

    Loads a workspace from a NeXus Processed entry in a NeXus file.
    LoadNexusProcessed is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of the input NeXus file (must exist) </LI>
    <LI> InputWorkspace - The name of the workspace to put the data </LI>
    </UL>

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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadNexusProcessed : public API::IFileLoader<Kernel::NexusDescriptor>
    {

    public:
      /// Default constructor
      LoadNexusProcessed();
      /// Destructor
      ~LoadNexusProcessed();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadNexusProcessed";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Nexus";}

      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::NexusDescriptor & descriptor) const;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();

      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      /// Load a single entry
      API::Workspace_sptr loadEntry(Mantid::NeXus::NXRoot & root, const std::string & entry_name,
                                              const double& progressStart, const double& progressRange);

      API::Workspace_sptr loadTableEntry(Mantid::NeXus::NXEntry& entry);

	  API::Workspace_sptr loadPeaksEntry(Mantid::NeXus::NXEntry & entry);

      API::MatrixWorkspace_sptr loadEventEntry(Mantid::NeXus::NXData & wksp_cls,Mantid::NeXus::NXDouble & xbins,
          const double& progressStart, const double& progressRange);

      /// Read the data from the sample group
      void readSampleGroup(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      /// Add a property to the sample object
      bool addSampleProperty(Mantid::NeXus::NXMainClass & sample_entry, const std::string & entryName, API::Sample& sampleDetails);
      /// Read the spectra 
      void readInstrumentGroup(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      /// Splits a string of exactly three words into the separate words
      void getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3);
      /// Splits a string of exactly four words into the separate words
      void getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3, std::string & w4);

      ///Read the bin masking information
      void readBinMasking(Mantid::NeXus::NXData & wksp_cls, API::MatrixWorkspace_sptr local_workspace);

      /// Load a block of data into the workspace where it is assumed that the x bins have already been cached
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data,
                     Mantid::NeXus::NXDataSetTyped<double> & errors,
                     Mantid::NeXus::NXDataSetTyped<double> & farea,
                     bool hasFArea,
                     int64_t blocksize, int64_t nchannels, int64_t &hist,
                     API::MatrixWorkspace_sptr local_workspace);

      /// Load a block of data into the workspace where it is assumed that the x bins have already been cached
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data,
                     Mantid::NeXus::NXDataSetTyped<double> & errors,
                     Mantid::NeXus::NXDataSetTyped<double> & farea,
                     bool hasFArea,
                     int64_t blocksize, int64_t nchannels,
                     int64_t &hist, int64_t& wsIndex,
                     API::MatrixWorkspace_sptr local_workspace);
      /// Load a block of data into the workspace
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data,
                     Mantid::NeXus::NXDataSetTyped<double> & errors,
                     Mantid::NeXus::NXDataSetTyped<double> & farea,
                     bool hasFArea,
                     Mantid::NeXus::NXDouble & xbins,
                     int64_t blocksize, int64_t nchannels,
                     int64_t &hist, int64_t& wsIndex,
                     API::MatrixWorkspace_sptr local_workspace);

      /// Load the data from a non-spectra axis (Numeric/Text) into the workspace
      void loadNonSpectraAxis(API::MatrixWorkspace_sptr local_workspace, Mantid::NeXus::NXData & data);

      /// Validates the optional 'spectra to read' properties, if they have been set
      void checkOptionalProperties(const std::size_t numberofspectra);

      /// calculates the workspace size
      std::size_t calculateWorkspacesize(const std::size_t numberofspectra);
     
      /// Does the current workspace have uniform binning
      bool m_shared_bins;
      /// The cached x binning if we have bins
      MantidVecPtr m_xbins;
      /// Numeric values for the second axis, if applicable
      MantidVec m_axis1vals;

      /// Flag set if list of spectra to save is specifed
      bool m_list;
      /// Flag set if interval of spectra to write is set
      bool m_interval;
      /// The value of the spectrum_list property
      std::vector<int64_t> m_spec_list;
      /// The value of the spectrum_min property
      int64_t m_spec_min;
      /// The value of the spectrum_max property
      int64_t m_spec_max;

      // C++ interface to the NXS file
      ::NeXus::File * m_cppFile;
    };
	/// to sort the algorithmhistory vector
	bool UDlesserExecCount(Mantid::NeXus::NXClassInfo elem1, Mantid::NeXus::NXClassInfo elem2);

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_*/
