#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYCOMMANDARGUMENTS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYCOMMANDARGUMENTS_H_

#include <unordered_map>
#include <string>
#include <utility>

namespace {
typedef std::unordered_map<std::string, std::pair<std::string, std::string>>
    CommandArguments;
}
namespace MantidQt {
namespace CustomInterfaces {

class TomographyCommandArguments : public CommandArguments {
public:
  // need accessors for
  // argument -> pair.first
  // value -> pair.second
  // pair.second <- value(type)
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYCOMMANDARGUMENTS_H_