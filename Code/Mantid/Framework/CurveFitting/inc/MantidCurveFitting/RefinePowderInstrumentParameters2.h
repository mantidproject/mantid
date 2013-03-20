#ifndef MANTID_CURVEFITTING_RefinePowderInstrumentParameters2_H_
#define MANTID_CURVEFITTING_RefinePowderInstrumentParameters2_H_

#include "MantidKernel/System.h"

#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidCurveFitting/LeBailFit.h"
#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  /** RefinePowderInstrumentParameters2 : TODO: DESCRIPTION
    
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport RefinePowderInstrumentParameters2 : public Algorithm
  {
  public:
    RefinePowderInstrumentParameters2();
    virtual ~RefinePowderInstrumentParameters2();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "RefinePowderInstrumentParameters";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 3;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();

    /// Implement abstract Algorithm methods
    void init();

    /// Implement abstract Algorithm methods
    void exec();

    /// Fit instrument parameters by non Monte Carlo algorithm
    double execFitParametersNonMC();

    /// Refine instrument parameters by Monte Carlo/simulated annealing method
    double execFitParametersMC();

    /// Do MC/simulated annealing to refine parameters
    double doSimulatedAnnealing(map<string, Parameter> inparammap);

    /// Set up Monte Carlo random walk strategy
    void setupRandomWalkStrategy(map<string, Parameter>& parammap,
                                 vector<vector<string> >& mcgroups);

    /// Add parameter (to a vector of string/name) for MC random walk
    void addParameterToMCMinimize(vector<string>& parnamesforMC, string parname,
                                  map<string, Parameter> parammap);

    /// Propose new parameters
    void proposeNewValues(vector<string> mcgroup, map<string, Parameter>& curparammap,
                          map<string, Parameter>& newparammap, double currchisq);

    /// Determine whether the proposed value should be accepted or denied
    bool acceptOrDenyChange(double curchisq, double newchisq, double temperature);

    /// Book keep the best fitting result
    void bookKeepMCResult(map<string, Parameter> parammap, double chisq, int istep, int igroup, map<string, Parameter> &bestparammap);
    // vector<pair<double, map<string, Parameter> > > &bestresults, size_t maxnumresults);

    /// Implement parameter values, calculate function and its chi square.
    double calculateFunction(map<string, Parameter> parammap, vector<double>& vecY);

    /// Calculate Chi^2 of the a function with all parameters are fixed
    double calculateFunctionError(IFunction_sptr function, Workspace2D_sptr dataws,
                                  int wsindex);

    /// Fit function by non MC minimzer(s)
    double fitFunction(IFunction_sptr function, Workspace2D_sptr dataws, int wsindex, bool powerfit);

    /// Fit function (single step)
    bool doFitFunction(IFunction_sptr function, Workspace2D_sptr dataws, int wsindex,
                       string minimizer, int numiters, double &chi2, string &fitstatus);

    /// Process input properties
    void processInputProperties();

    /// Parse TableWorkspaces
    void parseTableWorkspaces();

    /// Parse table workspace to a map of Parameters
    void parseTableWorkspace(TableWorkspace_sptr tablews, map<string, Parameter> &parammap);

    /// Set parameter values to function from Parameter map
    void setFunctionParameterValues(IFunction_sptr function, map<string, Parameter> params);

    /// Update parameter values to Parameter map from fuction map
    void updateFunctionParameterValues(IFunction_sptr function, map<string, Parameter> &params);

    /// Set parameter fitting setup (boundary, fix or unfix) to function from Parameter map
    void setFunctionParameterFitSetups(IFunction_sptr function, map<string, Parameter> params);

    /// Construct output
    Workspace2D_sptr genOutputWorkspace(FunctionDomain1DVector domain, FunctionValues rawvalues);

    /// Construct an output TableWorkspace for refined peak profile parameters
    TableWorkspace_sptr genOutputProfileTable(map<string, Parameter> parameters,
                                              double startchi2, double finalchi2);

    /// Add a parameter to parameter map.  If this parametere does exist, then replace the value of it
    void addOrReplace(map<string, Parameter>& parameters, string parname, double parvalue);

    //--------  Variables ------------------------------------------------------
    /// Data workspace containg peak positions
    Workspace2D_sptr m_dataWS;

    /// Workspace index of the peak positions
    int m_wsIndex;

    /// TableWorkspace containing peak parameters value and fit information
    TableWorkspace_sptr m_paramTable;

    /// Fit mode
    enum {FIT, MONTECARLO} m_fitMode;

    /// Standard error mode
    enum {CONSTANT, USEINPUT} m_stdMode;

    /// Monte Carlo random walk steps
    int m_numWalkSteps;

    /// Random seed
    int m_randomSeed;

    /// Data structure (map of Parameters) to hold parameters
    map<string, Parameter> m_profileParameters;

    /// My function for peak positions
    ThermalNeutronDtoTOFFunction_sptr m_positionFunc;

    /// Damping factor
    double m_dampingFactor;

    /// Book keep for MC
    double m_bestChiSq;
    int m_bestChiSqStep;
    int m_bestChiSqGroup;

  };

  //================================= External Functions =========================================
  /// Convert a vector to a lookup map (dictionary)
  void convertToDict(vector<string> strvec, map<string, size_t>& lookupdict);

  /// Get the index from lookup dictionary (map)
  int getStringIndex(map<string, size_t> lookupdict, string key);

  /// Store function parameter values to a map
  void storeFunctionParameterValue(IFunction_sptr function, map<string, pair<double, double> > &parvaluemap);

  /// Restore function parameter values to a map
  void restoreFunctionParameterValue(map<string, pair<double, double> > parvaluemap, IFunction_sptr function,
                                     map<string, Parameter> &parammap);

  /// Copy parameters from source to target
  void duplicateParameters(map<string, Parameter> source, map<string, Parameter> &target);

  /// Copy parameters values from source to target
  void copyParametersValues(map<string, Parameter> source, map<string, Parameter>& target);

  /// Calculate Chi^2
  double calculateFunctionChiSquare(const vector<double> modelY, const vector<double> dataY,
                                    const vector<double> dataE);

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_RefinePowderInstrumentParameters2_H_ */
