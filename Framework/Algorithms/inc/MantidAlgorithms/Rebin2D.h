#ifndef MANTID_ALGORITHMS_REBIN2D_H_
#define MANTID_ALGORITHMS_REBIN2D_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
Rebins both axes of a two-dimensional workspace to the given parameters.

@author Martyn Gigg, Tessella plc

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class DLLExport Rebin2D : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "Rebin2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebins both axes of a 2D workspace using the given parameters";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "SofQW"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Rebin"; }

protected:
  /// Progress reporter
  boost::shared_ptr<API::Progress> m_progress;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Setup the output workspace
  API::MatrixWorkspace_sptr
  createOutputWorkspace(const API::MatrixWorkspace_const_sptr &parent,
                        HistogramData::BinEdges &newXBins,
                        HistogramData::BinEdges &newYBins,
                        const bool useFractionalArea) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBIN2D_H_ */
