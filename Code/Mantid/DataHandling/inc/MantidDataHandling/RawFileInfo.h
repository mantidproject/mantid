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
   An algorithm to extract details of the  RPB_STRUCT structure within a RAW file
   
   Required properties:
   <UL>
   <LI>Filename - The raw file to use to gather the information</LI>
   <LI>OutputTableName - The name of the TableWorkspace to output the parameters</LI>
   </UL>
   
   @author Martyn, Tessella plc
   @date 29/07/2009
   
   Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

}
}

#endif /*MANTIDDATAHANDLING_RAWFILEINFO_H_*/
