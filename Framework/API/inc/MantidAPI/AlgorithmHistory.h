// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidNexus/NeXusFile.hpp"

#include <ctime>
#include <set>
#include <vector>

namespace Mantid {
namespace API {
class IAlgorithm;
class Algorithm;
class AlgorithmHistory;

// typedefs for algorithm history pointers
using AlgorithmHistory_sptr = std::shared_ptr<AlgorithmHistory>;
using AlgorithmHistory_const_sptr = std::shared_ptr<const AlgorithmHistory>;
using AlgorithmHistories = std::vector<AlgorithmHistory_sptr>;

/** @class AlgorithmHistory AlgorithmHistory.h API/MAntidAPI/AlgorithmHistory.h

    This class stores information about the Command History used by algorithms
   on a workspace.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008
    */

class MANTID_API_DLL AlgorithmHistory {

public:
  /// History container

  /// The date-and-time will be stored as the Mantid::Types::Core::DateAndTime
  /// type
  explicit AlgorithmHistory(const Algorithm *const alg,
                            const Types::Core::DateAndTime &start = Types::Core::DateAndTime::getCurrentTime(),
                            const double &duration = -1.0, std::size_t uexeccount = 0);
  AlgorithmHistory &operator=(const AlgorithmHistory &);
  AlgorithmHistory(const AlgorithmHistory &) = default;
  AlgorithmHistory(std::string name, int vers, std::string uuid,
                   const Types::Core::DateAndTime &start = Types::Core::DateAndTime::getCurrentTime(),
                   const double &duration = -1.0, std::size_t uexeccount = 0);
  void addExecutionInfo(const Types::Core::DateAndTime &start, const double &duration);
  void addProperty(const std::string &name, const std::string &value, bool isdefault,
                   const unsigned int &direction = 99);

  /// add a child algorithm history record to this history object
  void addChildHistory(const AlgorithmHistory_sptr &childHist);
  // get functions
  /// get name of algorithm in history const
  const std::string &name() const { return m_name; }
  /// get version number of algorithm in history const
  const int &version() const { return m_version; }
  /// get execution duration
  double executionDuration() const { return m_executionDuration; }
  /// get execution date
  Mantid::Types::Core::DateAndTime executionDate() const { return m_executionDate; }
  /// get the execution count
  const std::size_t &execCount() const { return m_execCount; }
  /// get the uuid
  const std::string &uuid() const { return m_uuid; }
  /// get parameter list of algorithm in history const
  const Mantid::Kernel::PropertyHistories &getProperties() const { return m_properties; }
  /// get the string representation of a specified property
  const std::string &getPropertyValue(const std::string &name) const;
  /// get the child histories of this history object
  const AlgorithmHistories &getChildHistories() const { return m_childHistories; }
  /// Retrieve a child algorithm history by index
  AlgorithmHistory_sptr getChildAlgorithmHistory(const size_t index) const;
  /// get storeInADS
  const bool &getStoreInADS() const { return m_storeInADS; }
  /// Add operator[] access
  AlgorithmHistory_sptr operator[](const size_t index) const;
  /// Retrieve the number of child algorithms
  size_t childHistorySize() const;
  /// print contents of object
  void printSelf(std::ostream &, const int indent = 0, const size_t maxPropertyLength = 0) const;
  /// Less than operator
  inline bool operator<(const AlgorithmHistory &other) const { return execCount() < other.execCount(); }
  /// Equality operator
  inline bool operator==(const AlgorithmHistory &other) const {
    return (execCount() == other.execCount() && name() == other.name());
  }
  /// Create a concrete algorithm based on a history record
  std::shared_ptr<IAlgorithm> createAlgorithm() const;
  /// Create an child algorithm from a history record at a given index
  std::shared_ptr<IAlgorithm> getChildAlgorithm(const size_t index) const;
  /// Write this history object to a nexus file
  void saveNexus(::NeXus::File *file, int &algCount) const;
  // Set the execution count
  void setExecCount(std::size_t execCount) { m_execCount = execCount; }
  /// Set data on history after it is created
  void fillAlgorithmHistory(const Algorithm *const alg, const Types::Core::DateAndTime &start, const double &duration,
                            std::size_t uexeccount);
  // Allow Algorithm::execute to change the exec count & duration after the
  // algorithm was executed
  friend class Algorithm;

private:
  // private constructor
  AlgorithmHistory();
  // Set properties of algorithm
  void setProperties(const Algorithm *const alg);
  /// The name of the Algorithm
  std::string m_name;
  /// The version of the algorithm
  int m_version{-1};
  /// The execution date of the algorithm
  Mantid::Types::Core::DateAndTime m_executionDate;
  /// The execution duration of the algorithm
  double m_executionDuration{-1.0};
  /// The PropertyHistory's defined for the algorithm
  Mantid::Kernel::PropertyHistories m_properties;
  /// count keeps track of execution order of an algorithm
  std::size_t m_execCount{0};
  /// set of child algorithm histories for this history record
  AlgorithmHistories m_childHistories;
  /// UUID for this algorithm history
  std::string m_uuid;
  /// If algorithm was set to store workspaces in the ADS
  bool m_storeInADS{true};
};

MANTID_API_DLL std::ostream &operator<<(std::ostream &, const AlgorithmHistory &);

} // namespace API
} // namespace Mantid
