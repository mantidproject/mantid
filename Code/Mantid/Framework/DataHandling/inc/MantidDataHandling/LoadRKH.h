#ifndef MANTID_DATAHANDLING_LOADRKH_H_
#define MANTID_DATAHANDLING_LOADRKH_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include <fstream>

namespace Mantid
{
namespace DataHandling
{
  /**
     Loads an RKH file into a Mantid 1D workspace

     Required properties:
     <UL>
     <LI> OutputWorkspace - The name output workspace.</LI>
     <LI> Filename - The path to the file in RKH format</LI>
     <LI> FirstColumnValue - The units of the first column in the file</LI>
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
     Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
     
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
     
     File change history is stored at: <https://github.com/mantidproject/mantid>
     Code Documentation is available at: <http://doxygen.mantidproject.org>    
  */
class DLLExport LoadRKH : public API::IFileLoader<Kernel::FileDescriptor>
{
public:
  /// Constructor
  LoadRKH() :m_unitKeys(), m_RKHKeys() {}
  /// Virtual destructor
  virtual ~LoadRKH() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadRKH"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Load a file written in the RKH format";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling\\Text;SANS"; }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor & descriptor) const;

private:
  
  /// Store the units known to the UnitFactory
  std::set<std::string> m_unitKeys;
  /// Store the units added as options for this algorithm
  std::set<std::string> m_RKHKeys;
  /// the input stream for the file being loaded
  std::ifstream m_fileIn;

  // Initialisation code
  void init();
  // Execution code
  void exec();

  bool is2D(const std::string & testLine);
  const API::MatrixWorkspace_sptr read1D();
  const API::MatrixWorkspace_sptr read2D(const std::string & firstLine);
  API::Progress read2DHeader(const std::string & initalLine, API::MatrixWorkspace_sptr & outWrksp, MantidVec & axis0Data);
  const std::string readUnit(const std::string & line);
  void readNumEntrys(const int nEntries, MantidVec & output);
  void binCenter(const MantidVec oldBoundaries, MantidVec & toCenter) const;

  // Remove lines from an input stream
  void skipLines(std::istream & strm, int nlines);

};

}
}
#endif /*MANTID_DATAHANDLING_LOADRKH_H_*/
