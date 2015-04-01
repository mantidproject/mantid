#ifndef MANTID_MDALGORITHMS_FOREGROUNDMODEL_H_
#define MANTID_MDALGORITHMS_FOREGROUNDMODEL_H_
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
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"

// Includes that each model will most probably require
// This lightens the load for the user model writer
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"

namespace Mantid {
//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
namespace PhysicalConstants {
class MagneticFormFactorTable;
}

namespace MDAlgorithms {
/**
 * Defines an interface to a Foreground model that is
 * to be combined with a resolution calculation.
 *
 * A concrete model should override the following functions
 *  - declareParameters()         : Defines the parameters within the model
 *  - scatteringIntensity() : Returns a value for the cross section with the
 *                            current parameters
 */
class DLLExport ForegroundModel : public API::ParamFunction {
public:
  /// Type of model
  enum ModelType { Broad, Sharp };

  /// Default constructor required by factory
  ForegroundModel();
  /// Constructor taking the fitted function to access the current parameter
  /// values
  ForegroundModel(const API::IFunction &fittingFunction);
  /// Destructor
  ~ForegroundModel();

  /// Function category
  virtual const std::string category() const { return "Quantification"; }

  /// Returns the type of model
  virtual ModelType modelType() const = 0;
  /// Calculates the intensity for the model for the current parameters, expt
  /// description & ND point
  virtual double
  scatteringIntensity(const API::ExperimentInfo &exptSetup,
                      const std::vector<double> &point) const = 0;

  /// Set a reference to the convolved fitting function. Needed as we need a
  /// default constructor
  void setFunctionUnderMinimization(const API::IFunction &fitFunction);
  /// Declares the parameters
  void declareParameters();
  /// Called when an attribute value is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &attr);
  /// Return the initial value of the parameter according to the fit by index
  double getInitialParameterValue(size_t index) const;
  /// Return the initial value of the parameter according to the fit by name
  double getInitialParameterValue(const std::string &name) const;
  /// Return the current parameter according to the fit by index
  double getCurrentParameterValue(const size_t index) const;
  /// Return the current parameter according to the fit by name
  double getCurrentParameterValue(const std::string &name) const;

protected:
  /// Returns a reference to the fitting function
  const API::IFunction &functionUnderMinimization() const;

  /// Set the default ion type for the form factor calculation
  void setFormFactorIon(const std::string &ionType);
  /// Returns the form factor for the given q^2 value
  double formFactor(const double qsqr) const;

  /// helper function used for fast conversion from qx,qy,qz coordinate system
  /// into hkl coordinate system
  static void convertToHKL(const API::ExperimentInfo &exptSetup,
                           const double &qx, const double &qy, const double &qz,
                           double &qh, double &qk, double &ql, double &arlu1,
                           double &arlu2, double &arlu3);

private:
  DISABLE_COPY_AND_ASSIGN(ForegroundModel)

  /// Required by the interface
  void function(const Mantid::API::FunctionDomain &,
                Mantid::API::FunctionValues &) const {}
  /// Add attributes common to all models
  void addAttributes();

  /// Hide these
  using ParamFunction::getParameter;

  /// A (non-owning) pointer to the function undergoing fitting
  const API::IFunction *m_fittingFunction;
  /// An offset for the number of parameters that were declared before this one
  size_t m_parOffset;

  /// the name of magnetic ion used to avoid resetting form factor table for the
  /// same ion
  std::string m_MagIonName;
  /// Owned pointer to magnetic form factor cache
  PhysicalConstants::MagneticFormFactorTable *m_formFactorTable;
};

/// boost::shared_ptr typedef
typedef boost::shared_ptr<ForegroundModel> ForegroundModel_sptr;
/// boost::shared_ptr to const typedef
typedef boost::shared_ptr<const ForegroundModel> ForegroundModel_const_sptr;
}
}

/*
 * Register a class into the factory using a global RegistrationHelper
 * in an anonymous namespace. The comma operator is used to call the
 * factory's subscribe method.
 */
#define DECLARE_FOREGROUNDMODEL(classname)                                     \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_alg_##classname(                 \
      ((Mantid::MDAlgorithms::ForegroundModelFactory::Instance()               \
            .subscribe<classname>(#classname)),                                \
       0));                                                                    \
  }

#endif /* MANTID_MDALGORITHMS_FOREGROUNDMODEL_H_ */
