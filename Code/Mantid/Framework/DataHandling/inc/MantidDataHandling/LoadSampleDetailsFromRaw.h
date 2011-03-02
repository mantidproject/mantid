#ifndef MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_
#define MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_

//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/**
   An algorithm to extract the sample details from the SPB structure within a RAW file
   
   Required properties:
   <UL>
   <LI>InputWorkspace - The workspace to add information to</LI>
   <LI>Filename - The raw file to use to gather the information</LI>
   </UL>
   
   @author Martyn, Tessella plc
   @date 29/07/2009
   
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
class DLLExport LoadSampleDetailsFromRaw : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  LoadSampleDetailsFromRaw() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~LoadSampleDetailsFromRaw() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadSampleDetailsFromRaw"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

}
}

#endif /*MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_*/
