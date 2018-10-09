// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ITRANSFORMSCALE_H_
#define MANTID_API_ITRANSFORMSCALE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include <string>
#include <vector>

#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace API {
/*Base class  representing a scaling transformation acting on a one-dimensional
  grid domain

  @author Jose Borreguero
  @date Aug/28/2012
*/

class MANTID_API_DLL ITransformScale {
public:
  ITransformScale() = default;
  /// Virtual destructor needed for an abstract class
  virtual ~ITransformScale() = default;
  virtual const std::string name() const { return "ITransformScale"; }
  /// The scaling transformation. Define in derived classes
  virtual void transform(std::vector<double> &gd) = 0;
}; // class ITransformScale

/// typedef for a shared pointer
using ITransformScale_sptr = boost::shared_ptr<ITransformScale>;

} // namespace API
} // namespace Mantid

#define DECLARE_TRANSFORMSCALE(classname)                                      \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_trs_##classname(                 \
      ((Mantid::API::TransformScaleFactory::Instance().subscribe<classname>(   \
           #classname)),                                                       \
       0));                                                                    \
  }

#endif /*MANTID_API_ITRANSFORMSCALE_H_*/
