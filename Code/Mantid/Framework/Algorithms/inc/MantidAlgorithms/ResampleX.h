#ifndef MANTID_ALGORITHMS_REBINRAGGED_H_
#define MANTID_ALGORITHMS_REBINRAGGED_H_

#include <map>
#include "MantidKernel/System.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** ResampleX : TODO: DESCRIPTION
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ResampleX  : public Algorithms::Rebin
  {
  public:
    ResampleX();
    virtual ~ResampleX();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string alias() const;
    
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Resample the x-axis of the data with the requested number of points.";}
    
    /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
    double determineBinning(MantidVec& xValues, const double xmin, const double xmax);
    /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
    void setOptions(const int numBins, const bool useLogBins, const bool isDist);

  private:
    const std::string workspaceMethodName() const { return ""; } // Override the one from Rebin to ignore us


    void init();
    void exec();

    std::map<std::string, std::string> validateInputs();
    bool m_useLogBinning;
    bool m_preserveEvents;
    int m_numBins;
    bool m_isDistribution;
    bool m_isHistogram;
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REBINRAGGED_H_ */
