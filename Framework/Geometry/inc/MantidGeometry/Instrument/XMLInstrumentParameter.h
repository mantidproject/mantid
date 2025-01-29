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
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/TimeROI.h"
#include <string>

namespace Mantid {

namespace Kernel {
template <typename TYPE> class TimeSeriesProperty;
}

namespace Geometry {
//--------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------
class IComponent;

/**
This class is used to store information about parameters in XML instrument
definition files and
instrument parameter files, so that such parameters can be added to a workspace
when appropriate.
For example log file parameters make reference to log file entry values in raw
data, and such
parameters needs raw data to be loaded first.

@author Anders Markvardsen, ISIS, RAL
@date 12/1/2009
*/
class MANTID_GEOMETRY_DLL XMLInstrumentParameter {
public:
  /// Default constructor
  XMLInstrumentParameter(std::string logfileID, std::string value, std::shared_ptr<Kernel::Interpolation> interpolation,
                         std::string formula, std::string formulaUnit, std::string resultUnit, std::string paramName,
                         std::string type, std::string tie, std::vector<std::string> constraint,
                         std::string &penaltyFactor, std::string fitFunc, std::string extractSingleValueAs,
                         std::string eq, const Geometry::IComponent *comp, double angleConvertConst,
                         const std::string &description, std::string visible);

  // XML attributes from instrument definition file or instrument parameter file
  const std::string m_logfileID; ///< logfile id
  const std::string m_value;     ///< rather then extracting value from logfile,
  /// specify a value directly
  const std::string m_paramName;               ///< parameter name
  const std::string m_type;                    ///< type of the data, e.g. int, double or string
  const std::string m_tie;                     ///< specific to fitting parameter specify any tie
  const std::vector<std::string> m_constraint; ///< specific to fitting
  /// parameter specify lower and
  /// upper bound in that order
  std::string m_penaltyFactor;         ///< specific to fitting parameter specify penalty factor
  const std::string m_fittingFunction; ///< specific to fitting parameter
  /// specify fitting function
  const std::string m_formula; ///< specific to fitting parameter. Specify
  /// formula to use for setting this parameter
  const std::string m_formulaUnit; ///< unit for formula (i.e. for Centre in formula)
  const std::string m_resultUnit;  ///< expected result (output) unit from
  /// evaluating the formula
  std::shared_ptr<Kernel::Interpolation> m_interpolation; ///< specific to fitting parameter
  const std::string m_extractSingleValueAs;               ///< describes the way to extract a
  /// single value from the log file(
  /// average, first number, etc)
  const std::string m_eq; ///< muParser equation to calculate the parameter
  /// value from the log value
  const Geometry::IComponent *m_component; ///< the component address

  /// Returns parameter value as generated using possibly equation expression
  /// etc
  double createParamValue(Mantid::Kernel::TimeSeriesProperty<double> const *logData, const Kernel::TimeROI *) const;

  /// when this const equals 1 it means that angle=degree (default) is set in
  /// IDF
  /// otherwise if this const equals 180/pi it means that angle=radian is set in
  /// IDF
  double m_angleConvertConst;
  /// if present, contains help string, describing the parameter
  const std::string m_description;
  /// if present, describes whether the parameter shall be visible in InstrumentViewer
  const std::string m_visible;
};

} // namespace Geometry
} // namespace Mantid
