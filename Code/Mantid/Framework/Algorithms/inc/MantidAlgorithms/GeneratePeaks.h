#ifndef MANTID_ALGORITHMS_GENERATEPEAKS_H_
#define MANTID_ALGORITHMS_GENERATEPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IBackgroundFunction.h"

namespace Mantid
{
namespace Algorithms
{

  /** GeneratePeaks : Generate peaks from a table workspace containing peak parameters
    
    @date 2012-04-10

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
  class DLLExport GeneratePeaks : public API::Algorithm
  {
  public:
    GeneratePeaks();
    virtual ~GeneratePeaks();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "GeneratePeaks";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Generate peaks in an output workspace according to a TableWorkspace containing a list of peak's parameters.";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Crystal";}

  private:

    void init();

    /// Implement abstract Algorithm methods
    void exec();

    /// Process algorithm properties
    void processAlgProperties(std::string &peakfunctype, std::string &bkgdfunctype);

    /// Process column names with peak parameter names
    void processTableColumnNames();

    void importPeaksFromTable(std::map<specid_t, std::vector<std::pair<double, API::IFunction_sptr> > >& functionmap);

    /// Import peak and background function parameters from vector
    void importPeakFromVector(std::vector<std::pair<double, API::IFunction_sptr> >& functionmap);


    /// Generate peaks in output data workspaces
    void generatePeaks(const std::map<specid_t, std::vector<std::pair<double, API::IFunction_sptr> > >& functionmap, API::MatrixWorkspace_sptr dataWS);

    /// Check whether function has a certain parameter
    bool hasParameter(API::IFunction_sptr function, std::string paramname);

    /// Create output workspace
    API::MatrixWorkspace_sptr createOutputWorkspace();

    API::MatrixWorkspace_sptr createDataWorkspace(std::vector<double> binparameters);

    void createFunction(std::string& peaktype, std::string& bkgdtype);

    void getSpectraSet(DataObjects::TableWorkspace_const_sptr peakParmsWS);


    /// Get the IPeakFunction part in the input function
    API::IPeakFunction_sptr getPeakFunction(API::IFunction_sptr infunction);

    /// Add function parameter names to
    std::vector<std::string> addFunctionParameterNames(std::vector<std::string> funcnames);

    /// Peak function
    API::IPeakFunction_sptr m_peakFunction;

    /// Background function
    API::IBackgroundFunction_sptr m_bkgdFunction;

    ///
    std::vector<double> m_vecPeakParamValues;
    ///
    std::vector<double> m_vecBkgdParamValues;

    /// Spectrum map from full spectra workspace to partial spectra workspace
    std::map<specid_t, specid_t> m_SpectrumMap;

    /// Set of spectra (workspace indexes) in the original workspace that contain peaks to generate
    std::set<specid_t> m_spectraSet;

    /// Flag to use automatic background (???)
    bool m_useAutoBkgd;

    /// Parameter table workspace
    DataObjects::TableWorkspace_sptr m_funcParamWS;

    /// Input workspace (optional)
    API::MatrixWorkspace_const_sptr inputWS;

    /// Flag whether the new workspace is exactly as input
    bool m_newWSFromParent;

    /// Binning parameters
    std::vector<double> binParameters;

    /// Flag to generate background
    bool m_genBackground;

    /// Flag to indicate parameter table workspace containing raw parameters names
    bool m_useRawParameter;

    /// Maximum chi-square to have peak generated
    double m_maxChi2;

    /// Number of FWHM for peak to extend
    double m_numPeakWidth;

    /// List of functions' parameters naems
    std::vector<std::string> m_funcParameterNames;

    /// Indexes of height, centre, width, a0, a1, and a2 in input parameter table
    int i_height, i_centre, i_width, i_a0, i_a1, i_a2;

    /// Flag to use parameter table workspace
    bool m_useFuncParamWS;

    /// Spectrum if only 1 peak is to plot
    int m_wsIndex;
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_GENERATEPEAKS_H_ */
