// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/cow_ptr.h"
#include <boost/make_shared.hpp>

/*
 Creates a cow_ptr in-place.
*/

namespace Mantid {

namespace Kernel {

template <class T, class... Args> inline cow_ptr<T> make_cow(Args &&... args) {
  return cow_ptr<T>(boost::make_shared<T>(std::forward<Args>(args)...));
}

} // namespace Kernel
} // namespace Mantid