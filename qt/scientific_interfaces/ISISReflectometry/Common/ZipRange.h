// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#define MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template <class... Containers>
auto zip_range(Containers &... containers)
    -> decltype(boost::make_iterator_range(
        boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
        boost::make_zip_iterator(boost::make_tuple(containers.end()...)))) {
  return {boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
          boost::make_zip_iterator(boost::make_tuple(containers.end()...))};
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
