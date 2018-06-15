#ifndef MANTID_DATAHANDLING_SAVEGDA_H_
#define MANTID_DATAHANDLING_SAVEGDA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <unordered_map>

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL SaveGDA : public API::Algorithm {
public:
  const std::string name() const override;

  const std::string summary() const override;

  int version() const override;

  const std::vector<std::string> seeAlso() const override;

  const std::string category() const override;

private:
  struct CalibrationParams {
    CalibrationParams(const double _difa, const double _difc,
                      const double _tzero);
    const double difa;
    const double difc;
    const double tzero;
  };

  const static std::string PROP_OUTPUT_FILENAME;
  const static std::string PROP_INPUT_WS;
  const static std::string PROP_PARAMS_FILENAME;
  const static std::string PROP_GROUPING_SCHEME;

  void init() override;

  void exec() override;

  std::map<std::string, std::string> validateInputs() override;

  std::vector<CalibrationParams> parseParamsFile() const;
};

} // DataHandling
} // Mantid

#endif // MANTID_DATAHANDLING_SAVEGDA_H_
