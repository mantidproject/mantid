#ifndef MANTID_ALGORITHMS_GENERATEPEAKS_H_
#define MANTID_ALGORITHMS_GENERATEPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace Algorithms
{

  /** GeneratePeaks : Generate peaks from a table workspace containing peak parameters
    
    @date 2012-04-10

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport GeneratePeaks : public API::Algorithm
  {
  public:
    GeneratePeaks();
    virtual ~GeneratePeaks();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "GeneratePeaks";};
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Algorithm";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    API::MatrixWorkspace_sptr createOutputWorkspace(std::set<specid_t> spectra, std::vector<double> mBinParameters);

    API::IFunction_sptr createFunction(std::string m_peakFuncType, const double height, const double centre,
        const double sigma, const double a0, const double a1, const double a2, const bool withPeak, const bool withBackground);

    void getSpectraSet(DataObjects::TableWorkspace_const_sptr peakParmsWS, std::set<specid_t>& spectra);

    void generatePeaks(API::MatrixWorkspace_sptr dataWS, DataObjects::TableWorkspace_const_sptr peakparameters,
       std::string peakfunction);

    double getTableValue(DataObjects::TableWorkspace_const_sptr tableWS, std::string colname, size_t index);

    std::map<specid_t, specid_t> mSpectrumMap;

    bool mGeneratePeak;
    bool mGenerateBackground;
    bool mNewWSFromParent;

    API::MatrixWorkspace_const_sptr mInputWS;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_GENERATEPEAKS_H_ */
