#ifndef MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_
#define MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid
{

namespace DataHandling
{
/**
 Loads a NeXus file that conforms to the TOFRaw instrument definition format and stores it in a 2D workspace.
 LoadTOFRawNeXus is an algorithm and as such inherits  from the Algorithm class, via DataHandlingCommand, and overrides
 the init() & exec() methods.

 Required Properties:
 <UL>
 <LI> Filename - The name of and path to the input Nexus file </LI>
 <LI> OutputWorkspace - The name of the workspace in which to store the imported data
 (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
 </UL>

 Optional Properties:
 <UL>
 <LI> SpectrumMin  - The  starting spectrum number</LI>
 <LI> SpectrumMax  - The  final spectrum number (inclusive)</LI>
 <LI> SpectrumList - An ArrayProperty of spectra to load</LI>
 </UL>

 @author Stuart Campbell, ORNL

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
class DLLExport LoadTOFRawNeXus : public API::Algorithm
{
public:
  /// Default Constructor
  LoadTOFRawNeXus();

  // Destructor
  virtual ~LoadTOFRawNeXus()
  {}

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const
  { return "LoadTOFRawNeXus";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const
  { return 1;}

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const
  { return "DataHandling";}

private:
  /// Overwrites Algorithm method.
  void init();

  /// Overwrites Algorithm method
  void exec();

  /// Validate the optional input properties
  void checkOptionalProperties();

  /// Run LoadInstrument as a subalgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr);

  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr, Mantid::NeXus::NXEntry & entry);

  /// The name and path of the input file
  std::string m_filename;
  /// The instrument name from Nexus
  std::string m_instrument_name;
  /// The sample name read from Nexus
  std::string m_samplename;

};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLINGLOADTOFRAWNEXUS_H_ */
