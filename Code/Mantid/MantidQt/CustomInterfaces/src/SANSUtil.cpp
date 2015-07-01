#include <MantidQtCustomInterfaces/SANSUtil.h>

namespace MantidQt {
namespace CustomInterfaces {

SANSUtil::SANSUtil() {}
SANSUtil::~SANSUtil() {}

/**
 * Create a Python string list from an input string.
 * @param input :: the string which needs to be parsed
 * @param delimiter :: the delimiter of the input string
 */
QString SANSUtil::createPythonStringList(QString input, QString delimiter) {
  QString formattedString = "[";
  QString finalizer = "]";
  QString quotationMark = "'";

  if (input.isEmpty()) {
    return formattedString + finalizer;
  } else {
    input.replace(" ", "");
    auto inputStringList = input.split(delimiter);

    for (auto it = inputStringList.begin(); it != inputStringList.end(); ++it) {
      if (!it->isEmpty()) {
        formattedString += quotationMark + it->trimmed() + quotationMark + delimiter;
      }
    }

    // Remove the last comma
    formattedString.remove(formattedString.length() - delimiter.length(),
                           delimiter.length());
    formattedString += finalizer;
  }

  return formattedString;
}
}
}