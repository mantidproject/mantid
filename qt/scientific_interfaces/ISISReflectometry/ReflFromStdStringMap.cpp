#include "ReflFromStdStringMap.h"
#include <algorithm>
#include <iterator>
namespace MantidQt {
namespace CustomInterfaces {
std::map<QString, QString>
fromStdStringMap(std::map<std::string, std::string> const &inMap) {
  std::map<QString, QString> out;
  std::transform(inMap.begin(), inMap.end(), std::inserter(out, out.begin()),
                 [](std::pair<std::string, std::string> const &kvp)
                     -> std::pair<QString, QString> {
                   return std::make_pair(QString::fromStdString(kvp.first),
                                         QString::fromStdString(kvp.second));
                 });
  return out;
}

std::vector<std::map<QString, QString>> fromStdStringVectorMap(
    std::vector<std::map<std::string, std::string>> const &inVectorMap) {
  std::vector<std::map<QString, QString>> out;
  std::transform(
      inVectorMap.begin(), inVectorMap.end(), std::back_inserter(out),
      [](std::map<std::string, std::string> const &map)
          -> std::map<QString, QString> { return fromStdStringMap(map); });
  return out;
}
} // namespace CustomInterfaces
} // namespace MantidQt
