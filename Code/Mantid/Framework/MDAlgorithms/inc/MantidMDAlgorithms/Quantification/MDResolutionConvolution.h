#ifndef MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTION_H_
#define MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTION_H_
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
#include "MantidKernel/System.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"

namespace Mantid {
//
// Forward decalarations
//
namespace API {
class IFunctionMD;
}
namespace MDAlgorithms {
/**
 * Defines an interface to a class that is capable of performing
 * a convolution of a resolution function with a foreground model
 * It implements the ParamFunction interface in order to be able
 * declare parameters that can be passed on to the fit
 *
 * A concrete convolution type should override the following functions
 *  - declareParameters()  : Defines the parameters within the resolution model
 *to be fitted
 *  - declareAttributes()  : Defines the attributes (non-fit parameters) within
 *the resolution model to be fitted
 *  - signal() : Returns the cross section convoluted with the instrument
 *resolution
 */
class DLLExport MDResolutionConvolution : public API::ParamFunction {
public:
  /// Default constructor required by the factory
  MDResolutionConvolution();
  /// Construct the object with a foreground model name and function undergoing
  /// a fit
  MDResolutionConvolution(const API::IFunctionMD &fittingFunction,
                          const std::string &fgModelName);
  /// Virtual destructor for a base class
  virtual ~MDResolutionConvolution() {}

  /// Function category
  virtual const std::string category() const { return "Quantification"; }

  /**
   * Called once before any fit/simulation is started to allow caching of
   * frequently used parameters
   * @param workspace :: The MD that will be used for the fit
   */
  virtual void preprocess(const API::IMDEventWorkspace_const_sptr &workspace) {
    UNUSED_ARG(workspace);
  }
  /// Called before a function evaluation begins
  virtual void functionEvalStarting() {}
  /// Called after a function evaluation is finished
  virtual void functionEvalFinished() {}
  /// Called before a partial derivative evaluation begins
  virtual void partialDerivStarting() {}
  /// Called after a partial derivative evaluation is finished
  virtual void partialDerivFinished() {}

  /**
   * Return the value of the cross-section convoluted with the resolution for an
   * event
   * @param box :: An interator pointing at the current box under examination
   * @param innerRunIndex :: The index into the run for this workspace
   * @param eventIndex :: An index of the current pixel in the box
   */
  virtual double signal(const API::IMDIterator &box,
                        const uint16_t innerRunIndex,
                        const size_t eventIndex) const = 0;

  /// Declares the parameters. Overridden here to ensure that concrete models
  /// override it
  void declareAttributes();
  /// Setup the reference to the function under fit (required for factory)
  void setFittingFunction(const API::IFunctionMD &fittingFunction);
  /// Set a pointer to a foreground model from a string name (required for
  /// factory)
  void setForegroundModel(const std::string &fgModelName);
  /// Override set attribute to pass attributes to the foreground model if not
  /// know
  /// on the convolution type
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &value);

  /// Returns a reference to the foreground model
  const ForegroundModel &foregroundModel() const;

protected:
  /// Returns the foreground model pointer
  const API::IFunctionMD &getFittingFunction() const;

private:
  DISABLE_COPY_AND_ASSIGN(MDResolutionConvolution)

  /// Required for function interface
  void function(const Mantid::API::FunctionDomain &,
                Mantid::API::FunctionValues &) const {};

  /// A reference to the main function under minimzation
  const API::IFunctionMD *m_fittingFunction;
  /// A pointer to the foreground model
  ForegroundModel *m_foreground;
};
}
}

/*
 * Register a class into the factory using a global RegistrationHelper
 * in an anonymous namespace. The comma operator is used to call the
 * factory's subscribe method.
 */
#define DECLARE_MDRESOLUTIONCONVOLUTION(classname, alias)                      \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_alg_##classname(                 \
      ((Mantid::MDAlgorithms::MDResolutionConvolutionFactory::Instance()       \
            .subscribe<classname>(alias)),                                     \
       0));                                                                    \
  }

#endif /* MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTION_H_ */
