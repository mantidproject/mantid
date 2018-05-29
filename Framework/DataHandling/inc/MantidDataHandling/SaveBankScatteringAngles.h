#ifndef MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLES_H_
#define MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL SaveBankScatteringAngles : public API::Algorithm {
public:
  const std::string name() const override;

  const std::string summary() const override;

  int version() const override;

  const std::vector<std::string> seeAlso() const override;

  const std::string category() const override;

private:
  const static std::string PROP_FILENAME;
  const static std::string PROP_INPUT_WS;

  void init() override;

  void exec() override;

  std::map<std::string, std::string> validateInputs() override;
};

} // DataHandling
} // Mantid

#endif // MANTID_DATAHANDLING_SAVEBANKSCATTERINGANGLES_H_
