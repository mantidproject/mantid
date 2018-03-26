#ifndef SAVEPHX_H_
#define SAVEPHX_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
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
 *     Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
 *Ridge National Laboratory & European Spallation Source
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
 *     File change history is stored at:
 *<https://github.com/mantidproject/mantid>
 *     Code Documentation is available at: <http://doxygen.mantidproject.org>
 *
 */

class DLLExport SavePHX : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SavePHX"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes the detector geometry information of a workspace into a PHX "
           "format file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveSPE"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\SPE;Inelastic\\DataHandling";
  }

  /** the method used in tests. It requested the ChildAlgorithm, which does the
     detectors
     *  position calculations to produce a target workspace. This workspace then
     can be retrieved
        from analysis data service and used to check  the results of the save
     algorithm. */
  void set_resulting_workspace(const std::string &ws_name) {
    det_par_ws_name = ws_name;
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// The name of the table workpsace with detectors positions used in tests
  std::string det_par_ws_name;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*SAVEPHX_H_*/
