// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/Property.h"
#include "PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** Support for a property that holds an array of values.
    Implemented as a PropertyWithValue that holds a vector of the desired type.
    This class is really a convenience class to aid in the declaration of the
    property - there's no problem directly using a PropertyWithValue of vector
   type.

    @author Russell Taylor, Tessella Support Services plc
    @date 27/02/2008
 */
template <typename T> class MANTID_KERNEL_DLL ArrayProperty : public PropertyWithValue<std::vector<T>> {
public:
  ArrayProperty(const std::string &name, std::vector<T> vec,
                const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const IValidator_sptr &validator,
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const std::string &values,
                const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);

  ArrayProperty(const ArrayProperty &);

  ArrayProperty<T> *clone() const override;

  // Unhide the base class assignment operator
  using PropertyWithValue<std::vector<T>>::operator=;

  std::string value() const override;

  std::string setValue(const std::string &value) override;
  // May want to add specialisation to the class later, e.g. setting just one
  // element of the vector

private:
  // This method is a workaround for the C4661 compiler warning in visual
  // studio. This allows the template declaration and definition to be separated
  // in different files. See stack overflow article for a more detailed
  // explanation:
  // https://stackoverflow.com/questions/44160467/warning-c4661no-suitable-definition-provided-for-explicit-template-instantiatio
  // https://stackoverflow.com/questions/33517902/how-to-export-a-class-derived-from-an-explicitly-instantiated-template-in-visual
  void visualStudioC4661Workaround();
};

template <> MANTID_KERNEL_DLL void ArrayProperty<int>::visualStudioC4661Workaround();

} // namespace Kernel
} // namespace Mantid
