// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace API {
/** An interface to a function with location, which here means a
    function for which the user may ask what is its centre and  height.
    Also allow the user to set these values. Setting the centre and height
    may e.g. be used to get better starting values for the fitting.

    @author Anders Markvardsen, ISIS, RAL
    @date 2/11/2009
*/
class MANTID_API_DLL IFunctionWithLocation : public virtual ParamFunction, public virtual IFunction1D {
public:
  /// Virtual destructor
  /// (avoids warnings about non-trivial move assignment in virtually inheriting
  /// classes)
  ~IFunctionWithLocation() override {}

  /// Returns the centre of the function, which may be something as simple as
  /// the centre of
  /// the fitting range in the case of a background function or peak shape
  /// function this
  /// return value reflects the centre of the peak
  virtual double centre() const = 0;

  /// Returns the height of the function. For a background function this may
  /// return an average height of the background. For a peak function this
  /// return value is the height of the peak
  virtual double height() const = 0;

  /// Sets the parameters such that centre == c
  virtual void setCentre(const double c) = 0;

  /// Sets the parameters such that height == h
  virtual void setHeight(const double h) = 0;

  /// Fix a parameter or set up a tie such that value returned
  /// by centre() is constant during fitting.
  /// @param isDefault :: If true fix centre by default:
  ///    don't show it in ties
  virtual void fixCentre(bool isDefault = false) {
    UNUSED_ARG(isDefault);
    throw std::runtime_error("Generic centre fixing isn't implemented for this function.");
  }

  /// Free the centre parameter.
  virtual void unfixCentre() { throw std::runtime_error("Generic centre fixing isn't implemented for this function."); }
};

} // namespace API
} // namespace Mantid
