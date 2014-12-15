#ifndef MANTID_CURVEFITTING_RefinePowderInstrumentParameters3_H_
#define MANTID_CURVEFITTING_RefinePowderInstrumentParameters3_H_

#include "MantidKernel/System.h"

#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidCurveFitting/LeBailFit.h"
#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

namespace Mantid
{
namespace CurveFitting
{

  /** RefinePowderInstrumentParameters3 : 
    
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
  class DLLExport RefinePowderInstrumentParameters3 : public API::Algorithm
  {
  public:
    RefinePowderInstrumentParameters3();
    virtual ~RefinePowderInstrumentParameters3();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "RefinePowderInstrumentParameters";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Parameters include Dtt1, Dtt1t, Dtt2t, Zero, Zerot. ";}


    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 3;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    
    /// Implement abstract Algorithm methods
    void init();

    /// Implement abstract Algorithm methods
    void exec();

    /// Fit instrument parameters by non Monte Carlo algorithm
    double execFitParametersNonMC();

    /// Refine instrument parameters by Monte Carlo/simulated annealing method
    double execFitParametersMC();

    /// Do MC/simulated annealing to refine parameters
    double doSimulatedAnnealing(std::map<std::string, Parameter> inparammap);

    /// Set up Monte Carlo random walk strategy
    void setupRandomWalkStrategy(std::map<std::string, Parameter>& parammap,
                                 std::vector<std::vector<std::string> >& mcgroups);

    /// Add parameter (to a vector of string/name) for MC random walk
    void addParameterToMCMinimize(std::vector<std::string>& parnamesforMC, std::string parname,
                                  std::map<std::string, Parameter> parammap);

    /// Propose new parameters
    void proposeNewValues(std::vector<std::string> mcgroup, std::map<std::string, Parameter>& curparammap,
                          std::map<std::string, Parameter>& newparammap, double currchisq);

    /// Determine whether the proposed value should be accepted or denied
    bool acceptOrDenyChange(double curchisq, double newchisq, double temperature);

    /// Book keep the best fitting result
    void bookKeepMCResult(std::map<std::string, Parameter> parammap, double chisq, int istep, int igroup, std::map<std::string, Parameter> &bestparammap);
    // vector<pair<double, map<string, Parameter> > > &bestresults, size_t maxnumresults);

    /// Implement parameter values, calculate function and its chi square.
    double calculateFunction(std::map<std::string, Parameter> parammap, std::vector<double>& vecY);

    /// Calculate Chi^2 of the a function with all parameters are fixed
    double calculateFunctionError(API::IFunction_sptr function, DataObjects::Workspace2D_sptr dataws,
                                  int wsindex);

    /// Fit function by non MC minimzer(s)
    double fitFunction(API::IFunction_sptr function, DataObjects::Workspace2D_sptr dataws, int wsindex, bool powerfit);

    /// Fit function (single step)
    bool doFitFunction(API::IFunction_sptr function, DataObjects::Workspace2D_sptr dataws, int wsindex,
                       std::string minimizer, int numiters, double &chi2, std::string &fitstatus);

    /// Process input properties
    void processInputProperties();

    /// Parse TableWorkspaces
    void parseTableWorkspaces();

    /// Parse table workspace to a map of Parameters
    void parseTableWorkspace(DataObjects::TableWorkspace_sptr tablews, std::map<std::string, Parameter> &parammap);

    /// Set parameter values to function from Parameter map
    void setFunctionParameterValues(API::IFunction_sptr function, std::map<std::string, Parameter> params);

    /// Update parameter values to Parameter map from fuction map
    void updateFunctionParameterValues(API::IFunction_sptr function, std::map<std::string, Parameter> &params);

    /// Set parameter fitting setup (boundary, fix or unfix) to function from Parameter map
    void setFunctionParameterFitSetups(API::IFunction_sptr function, std::map<std::string, Parameter> params);

    /// Construct output
    DataObjects::Workspace2D_sptr genOutputWorkspace(API::FunctionDomain1DVector domain, API::FunctionValues rawvalues);

    /// Construct an output TableWorkspace for refined peak profile parameters
    DataObjects::TableWorkspace_sptr genOutputProfileTable(std::map<std::string, Parameter> parameters,
                                              double startchi2, double finalchi2);

    /// Add a parameter to parameter map.  If this parametere does exist, then replace the value of it
    void addOrReplace(std::map<std::string, Parameter>& parameters, std::string parname, double parvalue);

    //--------  Variables ------------------------------------------------------
    /// Data workspace containg peak positions
    DataObjects::Workspace2D_sptr m_dataWS;

    /// Workspace index of the peak positions
    int m_wsIndex;

    /// TableWorkspace containing peak parameters value and fit information
    DataObjects::TableWorkspace_sptr m_paramTable;

    /// Fit mode
    enum {FIT, MONTECARLO} m_fitMode;

    /// Standard error mode
    enum {CONSTANT, USEINPUT} m_stdMode;

    /// Monte Carlo random walk steps
    int m_numWalkSteps;

    /// Random seed
    int m_randomSeed;

    /// Data structure (map of Parameters) to hold parameters
    std::map<std::string, Parameter> m_profileParameters;

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
  void convertToDict(std::vector<std::string> strvec, std::map<std::string, size_t>& lookupdict);

  /// Get the index from lookup dictionary (map)
  int getStringIndex(std::map<std::string, size_t> lookupdict, std::string key);

  /// Store function parameter values to a map
  void storeFunctionParameterValue(API::IFunction_sptr function, std::map<std::string, std::pair<double, double> > &parvaluemap);

  /// Restore function parameter values to a map
  void restoreFunctionParameterValue(std::map<std::string, std::pair<double, double> > parvaluemap,
                                     API::IFunction_sptr function,
                                     std::map<std::string, Parameter> &parammap);

  /// Copy parameters from source to target
  void duplicateParameters(std::map<std::string, Parameter> source, std::map<std::string, Parameter> &target);

  /// Copy parameters values from source to target
  void copyParametersValues(std::map<std::string, Parameter> source, std::map<std::string, Parameter>& target);

  /// Calculate Chi^2
  double calculateFunctionChiSquare(const std::vector<double> &modelY,
                                    const std::vector<double> &dataY,
                                    const std::vector<double> &dataE);

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_RefinePowderInstrumentParameters3_H_ */
