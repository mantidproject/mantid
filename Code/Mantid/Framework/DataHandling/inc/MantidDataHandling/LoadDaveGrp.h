#ifndef MANTID_DATAHANDLING_LOADDAVEGRP_H_
#define MANTID_DATAHANDLING_LOADDAVEGRP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"
#include <fstream>
#include <string>

namespace Mantid
{
namespace DataHandling
{
/** @class Mantid::DataHandling::LoadDaveGrp
     Reads the DAVE grouped ASCII format into a workspace.

     Properties:
     <UL>
     <LI> OutputWorkspace - The workspace name to produce. </LI>
     <LI> Filename - The filename for input </LI>
     <LI> XAxisUnits - The units for the x-axis </LI>
     <LI> YAxisUnits - The units for the y-axis </LI>
     <LI> IsMicroEV - The original file is in micro-electron-volts </LI>
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
class DLLExport LoadDaveGrp : public API::IDataFileChecker
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
  /**
   * Do a quick check that this file can be loaded
   *
   * @param filePath the location of and the file to check
   * @param nread number of bytes to read
   * @param header the first 100 bytes of the file as a union
   * @return true if the file can be loaded, otherwise false
   */
  virtual bool quickFileCheck(const std::string& filePath, std::size_t nread,
      const file_header& header);
  /**
   * Check the structure of the file and return a value between 0 and 100 of
   * how much this file can be loaded
   *
   * @param filePath the location of and the file to check
   * @return a confidence level indicator between 0 and 100
   */
  virtual int fileCheck(const std::string& filePath);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
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
