// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFO_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

namespace Mantid {
namespace API {

/**
    Implements a lazy-loading mechanism for the experimental information
    stored in a NeXus file.
  */
class MANTID_API_DLL FileBackedExperimentInfo : public ExperimentInfo {
public:
  FileBackedExperimentInfo(const std::string &filename,
                           const std::string &nxpath);
  ExperimentInfo *cloneExperimentInfo() const override;

private:
  void populateIfNotLoaded() const override;
  void populateFromFile() const;

  mutable bool m_loaded;
  std::string m_filename;
  std::string m_nxpath;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
