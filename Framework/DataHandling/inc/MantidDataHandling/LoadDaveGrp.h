#ifndef MANTID_DATAHANDLING_LOADDAVEGRP_H_
#define MANTID_DATAHANDLING_LOADDAVEGRP_H_

#include <fstream>
#include <string>
#include <vector>

#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace DataHandling {
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

     @author Michael Reuter ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
   National Laboratory
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

     File change history is stored at: <https://github.com/mantidproject/mantid>
     Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport LoadDaveGrp : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Constructor
  LoadDaveGrp();
  /// Algorithm's name
  const std::string name() const override { return "LoadDaveGrp"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a DAVE grouped ASCII file and stores it in a 2D "
           "workspace (Workspace2D class).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveDaveGrp"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Text;Inelastic\\DataHandling";
  }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialization code
  void init() override;
  /// Execution code
  void exec() override;
  /**
   * Function to retrieve the lengths of the x and y axes. This function uses
   * the same code for each call, but it is the order which determines the axis
   * found.
   *
   * @param length the size of a given axis
   */
  void getAxisLength(size_t &length);

  /**
   * Function to parse and store the actual axis values. Again, read order
   * determines the axis.
   *
   * @param axis the array to store the axis values
   * @param length the size of the axis
   */
  void getAxisValues(std::vector<double> &axis, const std::size_t length);

  /**
   * Function to parse and store the signal and errors from the data file.
   *
   * @param workspace handle to the workspace to to load data into
   */
  void getData(API::MatrixWorkspace_sptr workspace);

  /**
   * Function to setup the workspace ready for data to be loaded into it
   *
   * @return a new handle to the workspace
   */
  API::MatrixWorkspace_sptr setupWorkspace() const;

  /**
   * Function to set the workspace axes
   *
   * @param workspace handle to the workspace to set axes on
   * @param xAxis the x axis data
   * @param yAxis the y axis data
   */
  void setWorkspaceAxes(API::MatrixWorkspace_sptr workspace,
                        const std::vector<double> &xAxis,
                        const std::vector<double> &yAxis) const;

  /**
   * Convert a workspace to a histogram
   *
   * @param workspace handle to the distribution workspace to convert to a
   * histogram
   */
  API::MatrixWorkspace_sptr
  convertWorkspaceToHistogram(API::MatrixWorkspace_sptr workspace);

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
  std::size_t xLength;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADDAVEGRP_H_
