#ifndef MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#define MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#include <boost/iterator/zip_iterator.hpp>

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
}
}
#endif // MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
