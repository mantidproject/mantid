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
   * Function to retrieve the lengths of the x and y axes. This function uses
   * the same code for each call, but it is the order which determines the axis
   * found.
   *
   * @param length the size of a given axis
   */
  void getAxisLength(int &length);
  /**
   * Function to parse and store the actual axis values. Again, read order
   * determines the axis.
   *
   * @param axis the array to store the axis values
   * @param length the size of the axis
   */
  void getAxisValues(MantidVec *axis, const std::size_t length);
  /**
   * Function to parse and store the signal and errors from the data file.
   *
   * @param data the array to store the signal values
   * @param errs the array to store the error values
   */
  void getData(std::vector<MantidVec *> &data, std::vector<MantidVec *> &errs);
  /**
   * Function to read a line from the data file. Makes handling comment lines
   * easy.
   */
  void readLine();

  /// Handle for input data file
  std::ifstream ifile;
  /// Placeholder for file lines
  std::string line;
  /// The number of groups present in the data file
  std::size_t nGroups;
  /// The size of the x-axis in the data file
  int xLength;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADDAVEGRP_H_
