#ifndef MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_
#define MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid
{
namespace MDEvents
{

  class DLLExport IntegrateEllipsoids : public API::Algorithm
  {
  public:
    IntegrateEllipsoids();
    virtual ~IntegrateEllipsoids();
    
    virtual const std::string name() const;
    virtual int   version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    MDWSDescription m_targWSDescr;

    void initTargetWSDescr(DataObjects::EventWorkspace_sptr wksp);
  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_ */
