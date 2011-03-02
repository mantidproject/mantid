#ifndef MANTIDDATAHANDLING_RAWFILEINFO_H_
#define MANTIDDATAHANDLING_RAWFILEINFO_H_

//------------------------------------
// Includes
//------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/**
   An algorithm to extract pertinent information about a RAW file without loading the data.
   
   Required input properties:
   <UL>
   <LI> Filename - The raw file to use to gather the information </LI>
   <LI> GetRunParameters - Flag indicating whether to output run parameters (RPB_STRUCT) in a table (default false)</LI>
   </UL>
   
   Output properties:
   <UL>
   <LI> RunTitle         - The title of the run (r_title) </LI>
   <LI> RunHeader        - The run header (HDR_STRUCT) </LI>
   <LI> SpectraCount     - The number of spectra (t_nsp1) </LI>
   <LI> TimeChannelCount - The number of time channels (t_ntc1) </LI>
   <LI> PeriodCount      - The number of periods (t_nper) </LI>
   <LI> RunParameterTable (If requested by GetRunParameters flag above) <LI>
   </UL>
   
   @author Martyn, Tessella plc
   @date 29/07/2009
   
   Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport RawFileInfo : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  RawFileInfo() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~RawFileInfo() {}
  /// Algorithm's name
  virtual const std::string name() const { return "RawFileInfo"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

}
}

#endif /*MANTIDDATAHANDLING_RAWFILEINFO_H_*/
