#ifndef MANTID_CUSTOMINTERFACES_ROWTEMPLATE_H_
#define MANTID_CUSTOMINTERFACES_ROWTEMPLATE_H_
#include <boost/optional.hpp>
#include "ReductionOptionsMap.h"
#include "RangeInQ.h"
#include "../DllConfig.h"
#include <vector>
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RowTemplate {
public:
  RowTemplate(double theta, std::pair<std::string, std::string> tranmissionRuns,
              boost::optional<RangeInQ> qRange,
              boost::optional<double> scaleFactor,
              ReductionOptionsMap reductionOptions);

  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  double theta() const;
  boost::optional<RangeInQ> const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;

private:
  double m_theta;
  std::pair<std::string, std::string> m_transmissionRuns;
  boost::optional<RangeInQ> m_qRange;
  boost::optional<double> m_scaleFactor;
  ReductionOptionsMap m_reductionOptions;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_ROWTEMPLATE_H_
