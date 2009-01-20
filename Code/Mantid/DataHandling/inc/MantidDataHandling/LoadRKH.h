#ifndef MANTID_DATAHANDLING_LOADRKH_H_
#define MANTID_DATAHANDLING_LOADRKH_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

#include <istream>

namespace Mantid
{

namespace DataHandling
{
  /**
     Loads an RKH file into a Mantid 1D workspace

     Required properties:
     <UL>
     <LI> Filename - The path to the file in RKH format</LI>
     <LI> OutputWorkspace - The name output workspace.</LI>
     </UL>

     Optional Properties:
     <UL>
     <LI>DataStart - The line of data to start reading from</LI>
     <LI>DataEnd - The line of data to stop reading</LI>
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
     Copyright &copy; 2009 STFC Rutherford Appleton Laboratories
     
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

class LoadRKH : public Mantid::API::Algorithm
{
public:
  /// Constructor
  LoadRKH() : Mantid::API::Algorithm(), m_intTotalPoints(0), m_intReadStart(1), m_intReadEnd(1) {}
  /// Virtual destructor
  virtual ~LoadRKH() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadRKH"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  // Initialisation code
  void init();
  //Execution code
  void exec();

  //Check optional properties 
  void checkOptionalProperties();
  // Remove lines from an input stream
  void skipLines(std::istream & strm, int nlines);

  //Total number of lines in this set
  int m_intTotalPoints;
  //Line to start reading from
  int m_intReadStart;
  //Line to finish reading
  int m_intReadEnd;

  // Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

}

}
#endif /*MANTID_DATAHANDLING_LOADRKH_H_*/
