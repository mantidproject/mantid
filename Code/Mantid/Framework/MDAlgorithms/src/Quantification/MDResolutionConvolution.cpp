//
// Includes
//
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"

namespace Mantid {
namespace MDAlgorithms {
/// Default constructor required by the factory
MDResolutionConvolution::MDResolutionConvolution()
    : ParamFunction(), m_fittingFunction(NULL), m_foreground(NULL) {}

/**
 * Construct the object with a ForegroundModel & resolution the function being
 * fitted
 * @param fittingFunction :: A pointer to the function being fitted
 * @param fgModelName :: The name of the foreground model to use to calculate
 * the
 *                       scattering intensity
 */
MDResolutionConvolution::MDResolutionConvolution(
    const API::IFunctionMD &fittingFunction, const std::string &fgModelName)
    : ParamFunction(), m_fittingFunction(NULL), m_foreground(NULL) {
  setFittingFunction(fittingFunction);
  setForegroundModel(fgModelName);
}

/**
 * Setup the model & reference to the function under fit
 * @param fittingFunction :: A reference to the function undergoing a fit
 */
void MDResolutionConvolution::setFittingFunction(
    const API::IFunctionMD &fittingFunction) {
  m_fittingFunction = &fittingFunction;
}

/**
 * Set the name of the foreground model to use with the calculation
 * @param fgModelName :: The name of an existing model. Throws if the model is
 * not known
 */
void
MDResolutionConvolution::setForegroundModel(const std::string &fgModelName) {
  if (m_foreground)
    return; // All ready done
  if (!m_fittingFunction) {
    throw std::runtime_error("MDResolutionConvolution::setForegroundModel - "
                             "Fitting function must be defined first");
  }
  try {
    m_foreground = ForegroundModelFactory::Instance().createModel(
        fgModelName, getFittingFunction());
  } catch (Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("Unknown foreground model selected - \"" +
                                fgModelName + "\"");
  }
}

/// Returns a reference to the foreground model
const ForegroundModel &MDResolutionConvolution::foregroundModel() const {
  return *m_foreground;
}

/// Declares the parameters. Overridden here to ensure that concrete models
/// override it
void MDResolutionConvolution::declareAttributes() {
  throw Kernel::Exception::NotImplementedError(
      "Error: Override MDResolutionConvolution::declareAttributes() "
      "and declare convolution parameters");
}

/**
 * Set an attribute if the name is known to the convolution. If not, pass it on
 * to the foreground model
 * @param name :: The name of the attribute
 * @param value :: The value of the attribute
 */
void
MDResolutionConvolution::setAttribute(const std::string &name,
                                      const API::IFunction::Attribute &value) {
  if (this->hasAttribute(name)) {
    ParamFunction::setAttribute(name, value);
  } else {
    m_foreground->setAttribute(name, value);
  }
}

//-------------------------------------------------------------------------
// Protected members
//-------------------------------------------------------------------------
/**
 *  @return A pointer to the function undergoing fitting. Required to access
 *  the current parameters of the fit
 */
const API::IFunctionMD &MDResolutionConvolution::getFittingFunction() const {
  assert(m_fittingFunction);
  return *m_fittingFunction;
}

//-------------------------------------------------------------------------
// Private members
//-------------------------------------------------------------------------
}
}
