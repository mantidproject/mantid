#ifndef MANTID_SLICEVIEWER_FIRSTEXPERIMENTINFOQUERY_H_
#define MANTID_SLICEVIEWER_FIRSTEXPERIMENTINFOQUERY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidKernel/System.h"

namespace MantidQt {
namespace SliceViewer {
/*---------------------------------------------------------
FirstExperimentInfoQuery

Represents a query against the first experiment info of a workspace.
----------------------------------------------------------*/
class DLLExport FirstExperimentInfoQuery {
public:
  virtual bool hasOrientedLattice() const = 0;
  virtual bool hasRotatedGoniometer() const = 0;
};

/*---------------------------------------------------------
FirstExperimentInfoQueryAdapter

Templated adapter over the FirstExperimentInfoQuery interface.
----------------------------------------------------------*/
template <typename T>
class DLLExport FirstExperimentInfoQueryAdapter
    : public FirstExperimentInfoQuery {
public:
  using Adaptee_sptr = boost::shared_ptr<const T>;

private:
  Adaptee_sptr m_ws;

public:
  FirstExperimentInfoQueryAdapter(Mantid::API::IMDWorkspace_sptr ws) {
    m_ws = boost::dynamic_pointer_cast<const T>(ws);
    if (!m_ws) {
      throw std::invalid_argument(
          "Workspace object is of the wrong type for this adapter.");
    }
  }

  bool hasOrientedLattice() const {
    Mantid::API::MultipleExperimentInfos_const_sptr expInfos =
        boost::dynamic_pointer_cast<const Mantid::API::MultipleExperimentInfos>(
            m_ws);
    bool hasLattice = false;
    if (expInfos != NULL && expInfos->getNumExperimentInfo() > 0) {
      Mantid::API::ExperimentInfo_const_sptr expInfo =
          expInfos->getExperimentInfo(0);
      hasLattice = expInfo->sample().hasOrientedLattice();
    }
    return hasLattice;
  }

  bool hasRotatedGoniometer() const {
    Mantid::API::MultipleExperimentInfos_const_sptr expInfos =
        boost::dynamic_pointer_cast<const Mantid::API::MultipleExperimentInfos>(
            m_ws);
    bool hasRotatedGoniometer = false;
    if (expInfos != NULL && expInfos->getNumExperimentInfo() > 0) {
      Mantid::API::ExperimentInfo_const_sptr expInfo =
          expInfos->getExperimentInfo(0);
      hasRotatedGoniometer = expInfo->run().getGoniometerMatrix().isRotation();
    }
    return hasRotatedGoniometer;
  }
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_FIRSTEXPERIMENTINFOQUERY_H_ */
