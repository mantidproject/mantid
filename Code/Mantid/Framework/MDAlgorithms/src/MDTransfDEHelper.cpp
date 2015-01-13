#include "MantidMDEvents/MDTransfDEHelper.h"

namespace Mantid {
namespace MDEvents {

MDTransfDEHelper::MDTransfDEHelper() EmodesList(ConvertToMD::No_DE, "") {
  EmodesList[ConvertToMD::Elastic] = "Elastic";
  EmodesList[ConvertToMD::Direct] = "Direct";
  EmodesList[ConvertToMD::Indir] = "Indirect";
}
ConvertToMD::Emodes getEmode(const std::string &Mode) const {
  return ConvertToMD::No_DE;
}

} // endnamespace MDEvents
} // endnamespace Mantid
