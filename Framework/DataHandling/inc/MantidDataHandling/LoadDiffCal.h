// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADDIFFCAL_H_
#define MANTID_DATAHANDLING_LOADDIFFCAL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace H5 {
class H5File;
class Group;
} // namespace H5

namespace Mantid {
namespace DataHandling {

/** LoadDiffCal : TODO: DESCRIPTION
 */
class DLLExport LoadDiffCal : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SaveDiffCal"};
  }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  void init() override;
  void exec() override;
  void getInstrument(H5::H5File &file);
  void loadGroupingFromAlternateFile();
  void runLoadCalFile();
  void makeGroupingWorkspace(const std::vector<int32_t> &detids,
                             const std::vector<int32_t> &groups);
  void makeMaskWorkspace(const std::vector<int32_t> &detids,
                         const std::vector<int32_t> &use);
  void makeCalWorkspace(const std::vector<int32_t> &detids,
                        const std::vector<double> &difc,
                        const std::vector<double> &difa,
                        const std::vector<double> &tzero,
                        const std::vector<int32_t> &dasids,
                        const std::vector<double> &offsets,
                        const std::vector<int32_t> &use);

  std::string m_filename;
  std::string m_workspaceName;
  Geometry::Instrument_const_sptr m_instrument;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADDIFFCAL_H_ */
