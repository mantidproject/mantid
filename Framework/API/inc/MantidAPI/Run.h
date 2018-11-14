// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_RUN_H_
#define MANTID_API_RUN_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/LogManager.h"
#include "MantidKernel/TimeSplitter.h"

#include <vector>

namespace NeXus {
class File;
}

namespace Mantid {

namespace Kernel {
template <class T> class Matrix;
}

namespace Geometry {
class Goniometer;
}

namespace API {

/**
   This class stores information regarding an experimental run as a series
   of log entries

   @author Martyn Gigg, Tessella plc
   @date 22/07/2010
*/
class MANTID_API_DLL Run : public LogManager {
public:
  Run();
  Run(const Run &other);
  ~Run();
  Run &operator=(const Run &other);

  /// Clone
  boost::shared_ptr<Run> clone();

  /// Addition
  Run &operator+=(const Run &rhs);

  /// Filter the logs by time
  void filterByTime(const Types::Core::DateAndTime start,
                    const Types::Core::DateAndTime stop) override;
  /// Split the logs based on the given intervals
  void splitByTime(Kernel::TimeSplitterType &splitter,
                   std::vector<LogManager *> outputs) const override;

  /// Return an approximate memory size for the object in bytes
  size_t getMemorySize() const override;

  /// Set the proton charge
  void setProtonCharge(const double charge);
  /// Get the proton charge
  double getProtonCharge() const;
  /// Integrate the proton charge over the whole run time - default log
  /// proton_charge
  void
  integrateProtonCharge(const std::string &logname = "proton_charge") const;

  /// Store the given values as a set of histogram bin boundaries
  void storeHistogramBinBoundaries(const std::vector<double> &histoBins);
  /// Returns the bin boundaries for a given value
  std::pair<double, double> histogramBinBoundaries(const double value) const;
  /// Returns the vector of bin boundaries
  std::vector<double> getBinBoundaries() const;

  /// Set the gonoimeter & read the values from the logs if told to do so
  void setGoniometer(const Geometry::Goniometer &goniometer,
                     const bool useLogValues);
  /** @return A reference to the const Goniometer object for this run */
  inline const Geometry::Goniometer &getGoniometer() const {
    return *m_goniometer;
  }
  /** @return A reference to the non-const Goniometer object for this run */
  inline Geometry::Goniometer &mutableGoniometer() { return *m_goniometer; }

  // Retrieve the goniometer rotation matrix
  const Kernel::Matrix<double> &getGoniometerMatrix() const;

  /// Save the run to a NeXus file with a given group name
  void saveNexus(::NeXus::File *file, const std::string &group,
                 bool keepOpen = false) const override;
  /// Load the run from a NeXus file with a given group name
  void loadNexus(::NeXus::File *file, const std::string &group,
                 bool keepOpen = false) override;

private:
  /// Calculate the gonoimeter matrix
  void calculateGoniometerMatrix();

  /// Goniometer for this run
  std::unique_ptr<Geometry::Goniometer> m_goniometer;
  /// A set of histograms that can be stored here for future reference
  std::vector<double> m_histoBins;

  /// Adds all the time series in from one property manager into another
  void mergeMergables(Mantid::Kernel::PropertyManager &sum,
                      const Mantid::Kernel::PropertyManager &toAdd);
};
} // namespace API
} // namespace Mantid

#endif // MANTIDAPI_RUN_H_
