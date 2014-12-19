#ifndef MANTID_MDEVENTS_FAKEMDEVENTDATA_H_
#define MANTID_MDEVENTS_FAKEMDEVENTDATA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** FakeMDEventData : Algorithm to create fake multi-dimensional event
 * data that gets added to MDEventWorkspace, for use in testing.
 *
 * @author Janik Zikovsky
 * @date 2011-03-30 13:13:10.349627
 */
class DLLExport FakeMDEventData : public API::Algorithm {
public:
  FakeMDEventData();
  ~FakeMDEventData();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "FakeMDEventData"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Adds fake multi-dimensional event data to an existing "
           "MDEventWorkspace, for use in testing.\nYou can create a blank "
           "MDEventWorkspace with CreateMDWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Setup a detector cache for randomly picking IDs from the given workspace's
  /// instrument
  void setupDetectorCache(const API::IMDEventWorkspace &ws);
  /// Pick a detector ID for a particular event
  detid_t pickDetectorID();

  template <typename MDE, size_t nd>
  void addFakePeak(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);
  template <typename MDE, size_t nd>
  void
  addFakeUniformData(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  template <typename MDE, size_t nd>
  void addFakeRandomData(const std::vector<double> &params,
                         typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);
  template <typename MDE, size_t nd>
  void
  addFakeRegularData(const std::vector<double> &params,
                     typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// All detector IDs for this instrument
  std::vector<detid_t> m_detIDs;
  /// Random number generator
  boost::mt19937 m_randGen;
  /// Uniform distribution
  boost::uniform_int<size_t> m_uniformDist;
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDEVENTS_FAKEMDEVENTDATA_H_ */
