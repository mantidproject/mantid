// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/TransformScaleFactory.h"
#include "MantidAPI/ITransformScale.h"
#include "MantidKernel/Logger.h"

namespace Mantid::API {
namespace {
/// static logger
Kernel::Logger g_log("TransformScaleFactory");
} // namespace

/** Creates an instance of the appropriate scaling transform
 *  @param type The name of the scaling transform
 *  @returns A shared pointer to the created ITransformScale implementation
 *  @throws Exception::NotFoundError If the requested transform is not
 * registered
 */
ITransformScale_sptr TransformScaleFactoryImpl::create(const std::string &type) const {
  ITransformScale_sptr scaling;
  try {
    scaling = Kernel::DynamicFactory<ITransformScale>::create(type);
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.error("Error: Unable to create scaling transform of type " + type);
  }
  return scaling;
}

/** Override the DynamicFactory::createUnwrapped() method. We don't want it used
 * here.
 *  Making it private will prevent most accidental usage, though of course this
 * could
 *  be called through a DynamicFactory pointer or reference.
 *  @param className Argument that's ignored
 *  @returns Never
 *  @throws Exception::NotImplementedError every time!
 */
ITransformScale *TransformScaleFactoryImpl::createUnwrapped(const std::string &className) const {
  UNUSED_ARG(className)
  throw Kernel::Exception::NotImplementedError("Don't use this method - use the safe one!!!");
}

} // namespace Mantid::API
