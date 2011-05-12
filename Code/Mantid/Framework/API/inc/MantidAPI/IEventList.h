#ifndef MANTID_API_IEVENTLIST_H_
#define MANTID_API_IEVENTLIST_H_
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace API
{

  /// What kind of event list is being stored
  enum EventType {TOF, WEIGHTED, WEIGHTED_NOTIME};

  /** IEventList : Interface to Mantid::DataObjects::EventList class, used to
   * expose to PythonAPI
   * 
   * @author Janik Zikovsky
   * @date 2011-04-12 11:48:38.252379
   */
  class DLLExport IEventList 
  {
  public:
    virtual Mantid::API::EventType getEventType() const = 0;
    virtual void switchTo(Mantid::API::EventType newType) = 0;
    virtual void addDetectorID(const detid_t detID) = 0;
    virtual bool hasDetectorID(const detid_t detID) const = 0;
    virtual void clear(const bool removeDetIDs) = 0;
    virtual void reserve(size_t num) = 0;
    virtual bool isSortedByTof() const = 0;
    virtual std::size_t getNumberEvents() const = 0;
    virtual size_t getMemorySize() const = 0;
    virtual double integrate(const double minX, const double maxX, const bool entireRange) const = 0;
    virtual void convertTof(const double factor, const double offset=0.) = 0;
    virtual void scaleTof(const double factor) = 0;
    virtual void addTof(const double offset) = 0;
    virtual void addPulsetime(const double seconds) = 0;
    virtual void maskTof(const double tofMin, const double tofMax) = 0;
    virtual void getTofs(std::vector<double>& tofs) const = 0;
    virtual double getTofMin() const = 0;
    virtual double getTofMax() const = 0;
    virtual void setTofs(const MantidVec& tofs) = 0;
    virtual void multiply(const double value, const double error = 0.0) = 0;
    virtual void divide(const double value, const double error=0.0) = 0;
    virtual void multiply(const MantidVec & X, const MantidVec & Y, const MantidVec & E) = 0;
    virtual void divide(const MantidVec & X, const MantidVec & Y, const MantidVec & E) = 0;

  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_IEVENTLIST_H_ */
