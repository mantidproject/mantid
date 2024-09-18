// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/LogManager.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/SplittingInterval.h"

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
  bool operator==(const Run &other);
  bool operator!=(const Run &other);

  /// Clone
  std::shared_ptr<Run> clone();

  /// Addition
  Run &operator+=(const Run &rhs);

  /// Filter the logs by time
  void filterByTime(const Types::Core::DateAndTime start, const Types::Core::DateAndTime stop) override;
  void setTimeROI(const Kernel::TimeROI &timeroi) override;

  /// Return an approximate memory size for the object in bytes
  size_t getMemorySize() const override;

  /// Set the proton charge
  void setProtonCharge(const double charge);
  /// Get the proton charge
  double getProtonCharge() const;
  /// Integrate the proton charge over the whole run time - default log
  /// proton_charge
  void integrateProtonCharge(const std::string &logname = "proton_charge") const;
  /// determine the range of bad pulses to filter
  std::tuple<double, double, double> getBadPulseRange(const std::string &logname = "proton_charge",
                                                      const double &cutoff = 95.) const;

  /// update property "duration" with the duration of the Run's TimeROI attribute
  void setDuration();

  /// Store the given values as a set of histogram bin boundaries
  void storeHistogramBinBoundaries(const std::vector<double> &histoBins);
  /// Returns the bin boundaries for a given value
  std::pair<double, double> histogramBinBoundaries(const double value) const;
  /// Returns the vector of bin boundaries
  std::vector<double> getBinBoundaries() const;

  /// Set a single gonoimeter & read the average values from the logs if told to
  /// do so
  void setGoniometer(const Geometry::Goniometer &goniometer, const bool useLogValues);
  /// Set the gonoimeters using the individual values
  void setGoniometers(const Geometry::Goniometer &goniometer);
  /// Return reference to the first const Goniometer object for this run
  const Geometry::Goniometer &getGoniometer() const;

  /// Return reference to the first non-const Goniometer object for this run
  Geometry::Goniometer &mutableGoniometer();

  /// Retrieve the first goniometer rotation matrix
  const Kernel::Matrix<double> &getGoniometerMatrix() const;

  /// Return reference to a const Goniometer object for the run
  const Geometry::Goniometer &getGoniometer(const size_t index) const;
  /// Return reference to a non-const Goniometer object for the run
  Geometry::Goniometer &mutableGoniometer(const size_t index);
  /// Get the number of goniometers in the Run
  size_t getNumGoniometers() const;
  /// Retrieve the a goniometer rotation matrix
  const Kernel::Matrix<double> &getGoniometerMatrix(const size_t index) const;
  /// Append a goniometer to the run
  size_t addGoniometer(const Geometry::Goniometer &goniometer);
  /// Clear all goniometers on the Run
  void clearGoniometers();
  /// Get vector of all goniometer matrices in the Run
  const std::vector<Kernel::Matrix<double>> getGoniometerMatrices() const;

  /// Save the run to a NeXus file with a given group name
  void saveNexus(::NeXus::File *file, const std::string &group, bool keepOpen = false) const override;

  /// Load the run from a NeXus file with a given group name. Overload that uses NexusHDF5Descriptor for faster metadata
  /// lookup
  void loadNexus(::NeXus::File *file, const std::string &group, const Mantid::Kernel::NexusHDF5Descriptor &fileInfo,
                 const std::string &prefix, bool keepOpen = false) override;
  /// Load the run from a NeXus file with a given group name
  void loadNexus(::NeXus::File *file, const std::string &group, bool keepOpen = false) override;

private:
  /// Calculate the average gonoimeter matrix
  void calculateAverageGoniometerMatrix();
  /// Calculate the gonoimeter matrices from logs
  void calculateGoniometerMatrices(const Geometry::Goniometer &goniometer);

  /// Goniometer for this run
  std::vector<std::unique_ptr<Geometry::Goniometer>> m_goniometers;
  /// A set of histograms that can be stored here for future reference
  std::vector<double> m_histoBins;

  /// Adds all the time series in from one property manager into another
  void mergeMergables(Mantid::Kernel::PropertyManager &sum, const Mantid::Kernel::PropertyManager &toAdd);
  void copyGoniometers(const Run &other);

  // Function common to loadNexus overloads populating relevant members
  void loadNexusCommon(::NeXus::File *file, const std::string &nameClass);
};
} // namespace API
} // namespace Mantid
