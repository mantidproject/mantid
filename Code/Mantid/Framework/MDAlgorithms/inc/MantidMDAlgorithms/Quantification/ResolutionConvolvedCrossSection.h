#ifndef MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_
#define MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid {
namespace API {
/// Forward declarations
class ExperimentInfo;
}

namespace MDAlgorithms {
//
// Forward declarations
//
class MDResolutionConvolution;
class ForegroundModel;

/**
 * Defines a Fit function that calculates the convolution
 * of a foreground model with a resolution calculation for an MD workspace.
 */
class DLLExport ResolutionConvolvedCrossSection
    : public virtual API::ParamFunction,
      public virtual API::IFunctionMD {
public:
  /// Constructor
  ResolutionConvolvedCrossSection();
  /// Destructor
  ~ResolutionConvolvedCrossSection();
  /// Name for the function
  std::string name() const { return "ResolutionConvolvedCrossSection"; }
  /// Function category
  virtual const std::string category() const { return "Quantification"; }

  /// Declare the attributes associated with this function
  void declareAttributes();
  /// Declare model parameters.
  void declareParameters();

  /// Evaluate the function across the domain
  void function(const API::FunctionDomain &domain,
                API::FunctionValues &values) const;
  /// Return the signal contribution for the given box
  double functionMD(const API::IMDIterator &box) const;
  /// Store the simulated events in the given workspace
  void storeSimulatedEvents(const API::IMDEventWorkspace_sptr &resultWS);

private:
  /// Override the call to set the workspace here
  void setWorkspace(boost::shared_ptr<const API::Workspace> workspace);
  /// Fit is about to start
  void setUpForFit();
  /// Returns an estimate of the number of progress reports a single evaluation
  /// of the function will have.
  int64_t estimateNoProgressCalls() const;
  /// Set a value to a named attribute. Ensures additional parameters are set
  /// when foreground is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &value);
  /// Set a pointer to the concrete convolution object
  void setupResolutionFunction(const std::string &name,
                               const std::string &fgModelName);
  /// Mutex-locked version to store the function value
  void storeCalculatedWithMutex(const size_t index, const double signal,
                                API::FunctionValues &functionValues) const;

  /// Mutex to protect storing the function values
  mutable Poco::FastMutex m_valuesMutex;
  /// Flag that marks if this is a simulation store each event
  bool m_simulation;
  /// The meat of the calculation for each MD point
  MDResolutionConvolution *m_convolution;

  /// A pointer to the MD event workspace providing the data
  API::IMDEventWorkspace_const_sptr m_inputWS;

  /// Output events. Need to find a better way to handle other dimensions
  mutable std::list<MDEvents::MDEvent<4>> m_simulatedEvents;
};
}
}

#endif /* MANTID_MDALGORITHMS_RESOLUTIONCONVOLVEDCROSSSECTION_H_ */
