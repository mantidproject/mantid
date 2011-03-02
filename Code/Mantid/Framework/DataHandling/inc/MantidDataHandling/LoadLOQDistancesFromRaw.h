#ifndef MANTIDDATAHANDLING_LOADLOQDISTANCESFROMRAW_H_
#define MANTIDDATAHANDLING_LOADLOQDISTANCESFROMRAW_H_

//-------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{

/**
   An algorithm to extract geometry information specific to the LOQ instrument
   from a raw file

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
class DLLExport LoadLOQDistancesFromRaw : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  LoadLOQDistancesFromRaw() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~LoadLOQDistancesFromRaw() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadLOQDistancesFromRaw"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Run the MoveInstrumentComponent algorithm as a child algorithm
  void performMoveComponent(const std::string & comp_name, double zshift, 
			    double start_progress, double end_progress);
};

}
}

#endif /*MANTIDDATAHANDLING_LOADLOQDISTANCESFROMRAW_H_*/
