#ifndef DATAHANDING_SAVERKH_H_
#define DATAHANDING_SAVERKH_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace DataHandling
{
/**
     Saves a workspace in the RKH file format

     Required properties:
     <UL>
     <LI> Filename - The path save the file</LI>
     <LI> FirstColumnValue - The units of the first column in the file</LI>
     <LI> InputWorkspace - The name workspace to save.</LI>
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 26/01/2009
     
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
class DLLExport SaveRKH : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SaveRKH() : Mantid::API::Algorithm(), m_unitKeys(), m_RKHKeys() {}
  /// Virtual destructor
  virtual ~SaveRKH() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveRKH"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  ///Store the units known to the UnitFactory
  std::set<std::string> m_unitKeys;

  ///Store the units added as options for this algorithm
  std::set<std::string> m_RKHKeys;

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

}

}

#endif //DATAHANDING_SAVERKH_H_
