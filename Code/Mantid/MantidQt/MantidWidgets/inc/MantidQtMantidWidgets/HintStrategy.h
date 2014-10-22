#ifndef MANTID_MANTIDWIDGETS_HINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_HINTSTRATEGY_H

#include <map>
#include <string>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class HintStrategy
    {
    public:
      HintStrategy() {};
      virtual ~HintStrategy() {};

      //Provides a map of hints to be used by a HintingLineEdit widget
      virtual std::map<std::string,std::string> createHints() = 0;
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_HINTSTRATEGY_H */
