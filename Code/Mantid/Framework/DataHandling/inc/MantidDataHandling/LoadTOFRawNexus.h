#ifndef MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_
#define MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/IDataFileChecker.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{

namespace DataHandling
{
/**
 Loads a NeXus file that conforms to the TOFRaw instrument definition format and stores it in a 2D workspace.

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 */
class DLLExport LoadTOFRawNexus : public API::IDataFileChecker
{
public:
  /// Default Constructor
  LoadTOFRawNexus();

  // Destructor
  virtual ~LoadTOFRawNexus()
  {}

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const
  { return "LoadTOFRawNexus";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const
  { return 1;}

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const
  { return "DataHandling;DataHandling\\Nexus";}

  static std::string getEntryName(const std::string & filename);

  /// do a quick check that this file can be loaded
  bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
  /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
  int fileCheck(const std::string& filePath);

protected:
  void init();
  void initDocs();
  void exec();

  /// Validate the optional input properties
  void checkOptionalProperties();

  /// Run LoadInstrument as a subalgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr);

  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr, Mantid::NeXus::NXEntry & entry);

  void loadBank(const std::string &nexusfilename, const std::string & entry_name,
      const std::string &bankName, Mantid::API::MatrixWorkspace_sptr WS);

  void countPixels(const std::string &nexusfilename, const std::string & entry_name,
       std::vector<std::string> & bankNames);

  /// List of the absolute time of each pulse
  std::vector<Kernel::DateAndTime> pulseTimes;

  /// Map where key = detector ID, value = workspace index
  detid2index_map * id_to_wi;

  /// Signal # to load. Default 1
  int m_signal;

  /// Number of pixels
  size_t numPixels;

  /// Number of bins
  size_t numBins;

  /// Name of the 'data' field to load (depending on Signal)
  std::string m_dataField;

  /// Name of the 'axis' field to load (depending on Signal)
  std::string m_axisField;

  /// Units of the X axis found
  std::string m_xUnits;

  /// Mutex to avoid simultaneous file access
  Kernel::Mutex m_fileMutex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLINGLOADTOFRAWNEXUS_H_ */
