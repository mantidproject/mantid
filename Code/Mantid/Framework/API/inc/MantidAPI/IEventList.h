#ifndef MANTID_API_IEVENTLIST_H_
#define MANTID_API_IEVENTLIST_H_
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ISpectrum.h"

namespace Mantid {
namespace API {

/// What kind of event list is being stored
enum EventType { TOF, WEIGHTED, WEIGHTED_NOTIME };

/** IEventList : Interface to Mantid::DataObjects::EventList class, used to
 * expose to PythonAPI
 *
 * @author Janik Zikovsky
 * @date 2011-04-12 11:48:38.252379
 */
class MANTID_API_DLL IEventList : public ISpectrum {
public:
  /// Empty constructor
  IEventList() {}

  /// Constructor
  IEventList(specid_t specNo) : ISpectrum(specNo) {}

  /// Return the current event type for the list
  virtual Mantid::API::EventType getEventType() const = 0;
  /// Switch to a new event type within the list
  virtual void switchTo(Mantid::API::EventType newType) = 0;
  /// Clear the event list
  virtual void clear(const bool removeDetIDs) = 0;
  /// Reserve a fixed size for the list
  virtual void reserve(size_t num) = 0;
  /// IS the list sorted by TOF?
  virtual bool isSortedByTof() const = 0;
  /// Get the number of events from the list
  virtual std::size_t getNumberEvents() const = 0;
  /// Get memory size of event list
  virtual size_t getMemorySize() const = 0;
  /// Get copy of counts and errors, rebinned using on the given X values
  virtual void generateHistogram(const MantidVec &X, MantidVec &Y, MantidVec &E,
                                 bool skipError = false) const = 0;
  /// Get copy of counts and errors rebinned using the given X values w.r.t
  /// pulse time.
  virtual void generateHistogramPulseTime(const MantidVec &X, MantidVec &Y,
                                          MantidVec &E,
                                          bool skipError = false) const = 0;
  /// Get copy of counts and errors rebinning using the given X values w.r.t
  /// absolute time at the sample.
  virtual void generateHistogramTimeAtSample(const MantidVec &X, MantidVec &Y,
                                             MantidVec &E,
                                             const double &tofFactor,
                                             const double &tofOffset,
                                             bool skipError = false) const = 0;
  /// Integrate the event list
  virtual double integrate(const double minX, const double maxX,
                           const bool entireRange) const = 0;
  /// Convert the TOF values
  virtual void convertTof(const double factor, const double offset = 0.) = 0;
  /// Scale the TOF values by a constant
  virtual void scaleTof(const double factor) = 0;
  /// Add a value to the TOF values
  virtual void addTof(const double offset) = 0;
  /// Add a value to the pulse time values
  virtual void addPulsetime(const double seconds) = 0;
  /// Mask a given TOF range
  virtual void maskTof(const double tofMin, const double tofMax) = 0;
  /// Return the list of TOF values
  virtual std::vector<double> getTofs() const = 0;
  /// Return the list of TOF values
  virtual void getTofs(std::vector<double> &tofs) const = 0;
  /// Return the list of event weight  values
  virtual std::vector<double> getWeights() const = 0;
  /// Return the list of event weight values
  virtual void getWeights(std::vector<double> &weights) const = 0;
  /// Return the list of event weight error values
  virtual std::vector<double> getWeightErrors() const = 0;
  /// Return the list of event weight error values
  virtual void getWeightErrors(std::vector<double> &weightErrors) const = 0;
  /// Return the list of pulse time values
  virtual std::vector<Mantid::Kernel::DateAndTime> getPulseTimes() const = 0;
  /// Get the minimum TOF from the list
  virtual double getTofMin() const = 0;
  /// Get the maximum TOF from the list
  virtual double getTofMax() const = 0;
  /// Get the minimum pulse time from the list
  virtual Mantid::Kernel::DateAndTime getPulseTimeMin() const = 0;
  /// Get the maximum pulse time from the list
  virtual Mantid::Kernel::DateAndTime getPulseTimeMax() const = 0;
  /// Get the maximum time at sample.
  virtual Mantid::Kernel::DateAndTime
  getTimeAtSampleMax(const double &tofFactor,
                     const double &tofOffset) const = 0;
  /// Get the minimum time at sample
  virtual Mantid::Kernel::DateAndTime
  getTimeAtSampleMin(const double &tofFactor,
                     const double &tofOffset) const = 0;
  /// Set the TOFs from the given list
  virtual void setTofs(const MantidVec &tofs) = 0;
  /// Multiply event list by a constant with error
  virtual void multiply(const double value, const double error = 0.0) = 0;
  /// Divide event list by a constant with error
  virtual void divide(const double value, const double error = 0.0) = 0;
  /// Multiply event list by a histogram
  virtual void multiply(const MantidVec &X, const MantidVec &Y,
                        const MantidVec &E) = 0;
  /// Divide event list by a histogram
  virtual void divide(const MantidVec &X, const MantidVec &Y,
                      const MantidVec &E) = 0;
};

} // namespace Mantid
} // namespace API

#endif /* MANTID_API_IEVENTLIST_H_ */
