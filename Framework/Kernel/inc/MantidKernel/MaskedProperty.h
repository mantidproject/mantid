// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/PropertyWithValue.h"
#include <string>

/** A property class for masking the properties. It inherits from
   PropertyWithValue.
     This class masks the properties and useful when the property value is not
   to be
         displayed  in the user interface widgets like algorithm history
   display,
         log window etc and log file .The masked property will be displayed.
     This class is specialised for string and masks the property value with
   charcater "*"

    @class Mantid::Kernel::MaskedProperty
 */

namespace Mantid {
namespace Kernel {
template <typename TYPE = std::string>
class MaskedProperty : public Kernel::PropertyWithValue<TYPE> {
public:
  /// Constructor with a validator
  MaskedProperty(
      const std::string &name, TYPE defaultvalue,
      const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
      const unsigned int direction = Direction::Input);
  /// Constructor with a validator without validation
  MaskedProperty(const std::string &name, const TYPE &defaultvalue,
                 const unsigned int direction);

  MaskedProperty(const MaskedProperty &);

  /// "virtual" copy constructor
  MaskedProperty *clone() const override;

  /// Mask out the out the value in the history
  const Kernel::PropertyHistory createHistory() const override;

  /** This method returns the masked property value
   */
  TYPE getMaskedValue() const;

  // Unhide the PropertyWithValue assignment operator
  using Kernel::PropertyWithValue<TYPE>::operator=;

private:
  /// Perform the actual masking
  void doMasking() const;

  mutable TYPE m_maskedValue; ///< the masked value
};
} // namespace Kernel
} // namespace Mantid