#ifndef MANTID_MDALGORITHMS_COMPACTMD_H_
#define MANTID_MDALGORITHMS_COMPACTMD_H_

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/CutMD.h"
#include "MantidAPI/Algorithm.h"
#include "boost/shared_ptr.hpp"
namespace Mantid {
namespace MDAlgorithms {
    class DLLExport CompactMD : public API::DataProcessorAlgorithm{
public:
  CompactMD(){};
  ~CompactMD(){};

  virtual void init();
  virtual void exec();
  /// Algorithm's name for identification
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Compacts an MDWorkspace based on the first non-zero intensities";
  }
  /// Algorithm's version for identification
  virtual int version() const{return 1;}

  virtual Mantid::API::IMDHistoWorkspace_sptr
  compactWorkspace(Mantid::API::IMDHistoWorkspace_sptr workspace,
                   signal_t threshold);
};
}
}

#endif // MANTID_MDALGORITHMS_COMPACTMD_H_