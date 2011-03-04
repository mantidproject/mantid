#ifndef MANTID_DATAHANDLING_LOADDAVEGRP_H_
#define MANTID_DATAHANDLING_LOADDAVEGRP_H_

#include "MantidAPI/Algorithm.h"
#include <fstream>
#include <string>

namespace Mantid
{
namespace DataHandling
{
/** @class Mantid::DataHandling::LoadDaveGrp
     Reads the DAVE grouped ASCII format into a workspace.

     Required properties:
     <UL>
     <LI> OutputWorkspace - The workspace name to produce. </LI>
     <LI> Filename - The filename for input </LI>
     </UL>

     @author Michael Reuter ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     @date 25/02/2011

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
class DLLExport LoadDaveGrp: public Mantid::API::Algorithm
{
public:
  /// Constructor
  LoadDaveGrp();
  /// Virtual destructor
  virtual ~LoadDaveGrp() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadDaveGrp"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Initialization code
  void init();
  /// Execution code
  void exec();
  /**
   *
   */
  void getAxisLength(int &length);
  void getAxisValues(MantidVec *axis, const std::size_t length);
  void getData(std::vector<MantidVec *> &data, std::vector<MantidVec *> &errs);
  void readLine();

  std::ifstream ifile;
  std::string line;
  std::size_t nGroups;
  int xLength;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADDAVEGRP_H_
