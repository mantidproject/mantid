// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FOREGROUNDMODEL_H_
#define MANTID_MDALGORITHMS_FOREGROUNDMODEL_H_

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
  ~ForegroundModel() override;

  /// Disable copy operator
  ForegroundModel(const ForegroundModel &) = delete;

  /// Disable assignment operator
  ForegroundModel &operator=(const ForegroundModel &) = delete;

  /// Function category
  const std::string category() const override { return "Quantification"; }

  /// Returns the type of model
  virtual ModelType modelType() const = 0;
  /// Calculates the intensity for the model for the current parameters, expt
  /// description & ND point
  virtual double
  scatteringIntensity(const API::ExperimentInfo &exptSetup,
                      const std::vector<double> &point) const = 0;

  /// Set a reference to the convolved fitting function. Needed as we need a
  /// default constructor
  void setFunctionUnderMinimization(const API::IFunction &fittingFunction);
  /// Declares the parameters
  void declareParameters() override;
  /// Called when an attribute value is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &attr) override;
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
  /// Required by the interface
  void function(const Mantid::API::FunctionDomain &,
                Mantid::API::FunctionValues &) const override {}
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
  std::unique_ptr<PhysicalConstants::MagneticFormFactorTable> m_formFactorTable;
};

/// boost::shared_ptr typedef
using ForegroundModel_sptr = boost::shared_ptr<ForegroundModel>;
/// boost::shared_ptr to const typedef
using ForegroundModel_const_sptr = boost::shared_ptr<const ForegroundModel>;
} // namespace MDAlgorithms
} // namespace Mantid

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
