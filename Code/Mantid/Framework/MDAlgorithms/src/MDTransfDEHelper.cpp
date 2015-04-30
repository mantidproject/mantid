#include "MantidDataObjects/MDTransfDEHelper.h"

namespace Mantid {
namespace DataObjects {

MDTransfDEHelper::MDTransfDEHelper() EmodesList(ConvertToMD::No_DE, "") {
  EmodesList[ConvertToMD::Elastic] = "Elastic";
  EmodesList[ConvertToMD::Direct] = "Direct";
  EmodesList[ConvertToMD::Indir] = "Indirect";
}
ConvertToMD::Emodes getEmode(const std::string &Mode) const {
  return ConvertToMD::No_DE;
}

} // endnamespace DataObjects
} // endnamespace Mantid
