#ifndef SAVEPHX_H_
#define SAVEPHX_H_
/*WIKI* 


Saves the geometry information of the detectors in a workspace into a PHX format ASCII file.

The angular positions and angular sizes of the detectors are calculated using [[FindDetectorsPar]] algorithm. 

Mantid generated PHX file is an ASCII file consisting of the header and 7 text columns. Header contains the number of the rows in the phx file excluding the header. (number of detectors). The column has the following information about a detector:

  *         1st column      secondary flightpath,e.g. sample to detector distance (m) -- Mantid specific
  *         2nt             0
  *         3rd  "          scattering angle (deg)
  *         4th  "          azimuthal angle (deg)
  *                        (west bank = 0 deg, north bank = 90 deg etc.)
  *         5th  "          angular width e.g. delta scattered angle (deg) 
  *         6th  "          angular height e.g. delta azimuthal angle (deg)
  *         7th  "          detector ID    -- Mantid specific. 
  *---


In standard phx file only the columns  3,4,5 and 6 contain useful information. You should expect to find column 1 to be the secondary flightpath and the column 7 -- the detector ID in Mantid-generated phx files only. 





*WIKI*/

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/**
 *  Saves a workspace into an ASCII PHX file.
 *
 *   Required properties:
 *    <UL>
 *    <LI> InputWorkspace - The workspace name to save. </LI>
 *    <LI> Filename - The filename for output </LI>
 *    </UL>
 *
 *     @author Stuart Campbell, NScD, Oak Ridge National Laboratory
 *     @date 27/07/2010
 *
 *     Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
 *
 *     This file is part of Mantid.
 *
 *     Mantid is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Mantid is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *     File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 *     Code Documentation is available at: <http://doxygen.mantidproject.org>
 *
 */

class DLLExport SavePHX : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SavePHX() : Mantid::API::Algorithm()
  {}
  /// Virtual destructor
  virtual ~SavePHX()
  {}
  /// Algorithm's name
  virtual const std::string name() const
  { return "SavePHX";}
  /// Algorithm's version
  virtual int version() const
  { return (1);}
  /// Algorithm's category for identification
  virtual const std::string category() const
  { return "DataHandling;Inelastic";}

/** the method used in tests. It requested the subalgorithm, which does the detectors
   *  position calculations to produce a target workspace. This workspace then can be retrieved 
      from analysis data service and used to check  the results of the save algorithm. */
  void set_resulting_workspace(const std::string &ws_name){
      det_par_ws_name=ws_name;
  }
private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// The name of the table workpsace with detectors positions used in tests
  std::string det_par_ws_name;

};
} // namespace DataHandling
} // namespace Mantid

#endif /*SAVEPHX_H_*/
