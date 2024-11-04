// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <class... Containers>
auto zip_range(Containers &...containers)
    -> decltype(boost::make_iterator_range(boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
                                           boost::make_zip_iterator(boost::make_tuple(containers.end()...)))) {
  return {boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
          boost::make_zip_iterator(boost::make_tuple(containers.end()...))};
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
