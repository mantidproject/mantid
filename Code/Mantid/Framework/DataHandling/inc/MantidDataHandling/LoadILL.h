#ifndef MANTID_DATAHANDLING_LOADILL_H_
#define MANTID_DATAHANDLING_LOADILL_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid
{
namespace DataHandling
{
/**
  Loads an ILL nexus file into a Mantid workspace.

  Required properties:
  <UL>
  <LI> Filename - The ILL nexus file to be read </LI>
  <LI> Workspace - The name to give to the output workspace </LI>
  </UL>

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>    
 */
class DLLExport LoadILL : public API::IDataFileChecker 
{
public:
  /// Constructor
  LoadILL() : API::IDataFileChecker() {}
  /// Virtual destructor
  virtual ~LoadILL() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadILL"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling;Inelastic"; }
  ///checks the file can be loaded by reading 1st 100 bytes and looking at the file extension.
  bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
  /// check the structure of the file and if this file can be loaded return a value between 1 and 100
  int fileCheck(const std::string& filePath);
private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Initialisation code
  void init();
  // Execution code
  void exec();
  /// Load the counts
  void loadData(NeXus::NXData& dataGroup, API::MatrixWorkspace_sptr& workspace);
  /// Load the instrument form the IDF
  void runLoadInstrument(API::MatrixWorkspace_sptr workspace);

  std::string m_filename;       ///< The file to load
  std::string m_instrumentName; ///< Name of the instrumen
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadILL_H_*/
