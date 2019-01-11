// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef UPDATEABLEONDEMAND_H_
#define UPDATEABLEONDEMAND_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {
/**
 * Abstract class for types that can be forced to update themselves upon
 * request.
 */
class DLLExport UpdateableOnDemand {
public:
  // Force the implementation to update itself
  virtual void performUpdate() = 0;
  // Deliver a new peaks workspace for replacement of an existing one.
  virtual void updatePeaksWorkspace(
      const std::string &toName,
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) = 0;
  // Destructor
  virtual ~UpdateableOnDemand() {}
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* UPDATEABLEONDEMAND_H_ */
