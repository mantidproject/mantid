#ifndef MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/IDataFileChecker.h"

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadNexusProcessed : public API::IDataFileChecker 
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
      virtual const std::string category() const { return "DataHandling";}

      /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// specifies the order that algorithm data is listed in workspaces' histories
      enum AlgorithmHist
      {
        NAME = 0,                          //< algorithms name
        EXEC_TIME = 1,                     //< when the algorithm was run
        EXEC_DUR = 2,                      //< execution time for the algorithm
        PARAMS = 3                         //< the algorithm's parameters
      };

      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      /// Load a single entry
      API::MatrixWorkspace_sptr loadEntry(Mantid::NeXus::NXRoot & root, const std::string & entry_name,
                                              const double& progressStart, const double& progressRange);

      API::MatrixWorkspace_sptr loadEventEntry(Mantid::NeXus::NXData & wksp_cls,Mantid::NeXus::NXDouble & xbins,
          const double& progressStart, const double& progressRange);

      /// Read the data from the sample group
      void readSampleGroup(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      /// Add a property to the sample object
      bool addSampleProperty(Mantid::NeXus::NXMainClass & sample_entry, const std::string & entryName, API::Sample& sampleDetails);
      /// Read the spectra 
      void readInstrumentGroup(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      /// Read the algorithm history
      void readAlgorithmHistory(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      /// Splits a string of exactly three words into the separate words
      void getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3);
      /// Splits a string of exactly four words into the separate words
      void getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3, std::string & w4);
      ///Read the instrument parameter map
      void readParameterMap(Mantid::NeXus::NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace);
      ///Read the bin masking information
      void readBinMasking(Mantid::NeXus::NXData & wksp_cls, API::MatrixWorkspace_sptr local_workspace);
      /// Run LoadInstrument sub algorithm
      void runLoadInstrument(const std::string & inst_name, API::MatrixWorkspace_sptr local_workspace);
      /// Load a block of data into the workspace where it is assumed that the x bins have already been cached
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data, Mantid::NeXus::NXDataSetTyped<double> & errors, int blocksize,
		     int nchannels, int &hist, API::MatrixWorkspace_sptr local_workspace);

      /// Load a block of data into the workspace where it is assumed that the x bins have already been cached
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data, Mantid::NeXus::NXDataSetTyped<double> & errors, int blocksize,
		     int nchannels, int &hist,int& wsIndex, API::MatrixWorkspace_sptr local_workspace);
      /// Load a block of data into the workspace
      void loadBlock(Mantid::NeXus::NXDataSetTyped<double> & data, Mantid::NeXus::NXDataSetTyped<double> & errors, Mantid::NeXus::NXDouble & xbins,
		     int blocksize, int nchannels, int &hist,int& wsIndex, API::MatrixWorkspace_sptr local_workspace);

      /// Load the data from a non-spectra axis (Numeric/Text) into the workspace
      void loadNonSpectraAxis(API::MatrixWorkspace_sptr local_workspace, Mantid::NeXus::NXData & data);

      /// Validates the optional 'spectra to read' properties, if they have been set
      void checkOptionalProperties(const int numberofspectra);

      /// calculates the workspace size
      int calculateWorkspacesize(const int numberofspectra);
     
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
      std::vector<int> m_spec_list;
      /// The value of the spectrum_min property
      int m_spec_min;
      /// The value of the spectrum_max property
      int m_spec_max;
    };
	/// to sort the algorithm history vector
	bool UDlesserExecCount(Mantid::NeXus::NXClassInfo elem1, Mantid::NeXus::NXClassInfo elem2);

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_*/
