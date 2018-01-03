#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Convert an options map from a QMap to a std::map
 * @papram src [in] : the options as a QMap
 * @return : the options as a std::map
 */
OptionsMap convertOptionsFromQMap(const OptionsQMap &src) {
  OptionsMap dest;
  for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
      dest[iter.key()] = iter.value();
    }
  return dest;
}
}
}
}
