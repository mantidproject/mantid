#ifndef MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#define MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
#include <boost/iterator/zip_iterator.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template<class... Conts>
auto zip_range(Conts&... conts)
  -> decltype(boost::make_iterator_range(
  boost::make_zip_iterator(boost::make_tuple(conts.begin()...)),
  boost::make_zip_iterator(boost::make_tuple(conts.end()...))))
{
  return {boost::make_zip_iterator(boost::make_tuple(conts.begin()...)),
          boost::make_zip_iterator(boost::make_tuple(conts.end()...))};
}

}
}
#endif // MANTID_ISISREFLECTOMETRY_ZIPRANGE_H
