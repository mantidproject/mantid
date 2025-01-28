// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/UnitLabel.h"
#include <utility>

#include <unordered_map>
#include <vector>
#ifndef Q_MOC_RUN
#include <memory>
#endif

#include "tbb/concurrent_unordered_map.h"

namespace Mantid {
namespace Kernel {

enum class UnitParams { l2, twoTheta, efixed, delta, difa, difc, tzero };

// list of parameter names and values - some code may relay on map behaviour
// where [] creates element with value 0 if param name not present
using UnitParametersMap = std::unordered_map<UnitParams, double>;

/** The base units (abstract) class. All concrete units should inherit from
    this class and provide implementations of the caption(), label(),
    toTOF() and fromTOF() methods. They also need to declare (but NOT define)
    the unitID() method and register into the UnitFactory via the macro
   DECLARE_UNIT(classname).

    @author Russell Taylor, Tessella Support Services plc
    @date 25/02/2008
*/
class MANTID_KERNEL_DLL Unit {
public:
  /// (Empty) Constructor
  Unit();
  /// Virtual destructor
  virtual ~Unit() = default;

  /// @return a cloned instance of the other
  virtual Unit *clone() const = 0;

  /// The name of the unit. For a concrete unit, this method's definition is in
  /// the DECLARE_UNIT
  /// macro and it will return the argument passed to that macro (which is the
  /// unit's key in the
  /// factory).
  /// @return The unit ID
  virtual const std::string unitID() const = 0;
  /// The full name of the unit
  /// @return The unit caption
  virtual const std::string caption() const = 0;

  /// A label for the unit to be printed on axes, @see UnitLabel
  /// @return The unit label
  virtual const UnitLabel label() const = 0;

  /// Returns if the unit can be used in conversions
  /// @return true if the unit can be used in unit conversions
  virtual bool isConvertible() const { return true; }

  // Equality operators based on the value returned by unitID();
  bool operator==(const Unit &u) const;
  bool operator!=(const Unit &u) const;

  // Check whether the unit can be converted to another via a simple factor
  bool quickConversion(const Unit &destination, double &factor, double &power) const;
  bool quickConversion(std::string destUnitName, double &factor, double &power) const;

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata ::    The array of X data to be converted
   *  @param ydata ::    Not currently used (ConvertUnits passes an empty
   * vector)
   *  @param _l1 ::       The source-sample distance (in metres)
   *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param params ::  Map containing optional parameters eg
   *                    The sample-detector distance (in metres)
   *                    The scattering angle (in radians)
   *                    Fixed energy: EI (emode=1) or EF (emode=2)(in meV)
   *                    Delta (not currently used)
   */
  void toTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
             std::initializer_list<std::pair<const UnitParams, double>> params);
  void toTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
             const UnitParametersMap &params);

  /// Convert from the concrete unit to time-of-flight. TOF is in microseconds.
  double convertSingleToTOF(const double xvalue, const double &l1, const int &emode, const UnitParametersMap &params);

  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata ::    The array of X data to be converted
   *  @param ydata ::    Not currently used (ConvertUnits passes an empty
   * vector)
   *  @param _l1 ::       The source-sample distance (in metres)
   *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param params ::  Map containing optional parameters eg
   *                    The sample-detector distance (in metres)
   *                    The scattering angle (in radians)
   *                    Fixed energy: EI (emode=1) or EF (emode=2)(in meV)
   *                    Delta (not currently used)
   */
  void fromTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
               std::initializer_list<std::pair<const UnitParams, double>> params);

  void fromTOF(std::vector<double> &xdata, std::vector<double> &ydata, const double &_l1, const int &_emode,
               const UnitParametersMap &params);

  /// Convert from the time-of-flight to the concrete unit. TOF is in microseconds.
  double convertSingleFromTOF(const double xvalue, const double &l1, const int &emode, const UnitParametersMap &params);

  /// Initialize the unit to perform conversion using singleToTof() and singleFromTof()
  void initialize(const double &_l1, const int &_emode, const UnitParametersMap &params);

  /** Finalize the initialization. This will be overridden by subclasses as
   * needed. */
  virtual void init() = 0;

  /** Convert a single X value to TOF.
   * @param x value to convert
   * @return the TOF as converted.
   */
  virtual double singleToTOF(const double x) const = 0;

  /** Convert a single tof value to this unit
   * @param tof value to convert
   * @return the value in this unit as converted.
   */
  virtual double singleFromTOF(const double tof) const = 0;

  /// @return true if the unit was initialized and so can use singleToTOF()
  bool isInitialized() const { return initialized; }

  /// some units can be converted from TOF only in the range of TOF ;
  /// This function returns minimal TOF value still reversibly convertible into
  /// the unit.
  virtual double conversionTOFMin() const = 0;

  /// This function returns maximal TOF value still reversibly convertible into
  /// the unit.
  virtual double conversionTOFMax() const = 0;

  /**The range where conversion to TOF from given units is monotonic and
   * reversible*/
  virtual std::pair<double, double> conversionRange() const;

protected:
  // Add a 'quick conversion' for a unit pair
  void addConversion(std::string to, const double &factor, const double &power = 1.0) const;

  // validate the contents of the unit parameters map. Throw
  // std::invalid_argument if it's a global error or std::runtime_error if it's
  // a detector specific error
  virtual void validateUnitParams(const int emode, const UnitParametersMap &params);

  /// The unit values have been initialized
  bool initialized;
  /// l1 ::       The source-sample distance (in metres)
  double l1;
  /// emode ::    The energy mode (0=elastic, 1=direct geometry, 2=indirect
  /// geometry)
  int emode;
  /// additional parameters
  /// l2 :: distance from sample to detector (in metres)
  /// twoTheta :: scattering angle in radians
  /// efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
  /// difc :: diffractometer constant DIFC
  const UnitParametersMap *m_params;

private:
  /// A 'quick conversion' requires the constant by which to multiply the input
  /// and the power to which to raise it
  using ConstantAndPower = std::pair<double, double>;
  /// Lists, for a given starting unit, the units to which a 'quick conversion'
  /// can be made
  using UnitConversions = tbb::concurrent_unordered_map<std::string, ConstantAndPower>;
  /// The possible 'quick conversions' are held in a map with the starting unit
  /// as the key
  using ConversionsMap = tbb::concurrent_unordered_map<std::string, UnitConversions>;
  /// The table of possible 'quick conversions'
  static ConversionsMap s_conversionFactors;
};

/// Shared pointer to the Unit base class
using Unit_sptr = std::shared_ptr<Unit>;
/// Shared pointer to the Unit base class (const version)
using Unit_const_sptr = std::shared_ptr<const Unit>;

//----------------------------------------------------------------------
// Now the concrete units classes
//----------------------------------------------------------------------

/// The namespace for concrete units classes
namespace Units {

//=================================================================================================
/// Empty unit
class MANTID_KERNEL_DLL Empty : public Unit {
public:
  const std::string unitID() const override; ///< "Empty"
  const std::string caption() const override { return ""; }
  const UnitLabel label() const override;

  bool isConvertible() const override { return false; }
  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;

  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  Empty() : Unit() {}
};

//=================================================================================================
/// Label unit
class MANTID_KERNEL_DLL Label : public Empty {
public:
  const std::string unitID() const override; ///< "Label"
  const std::string caption() const override { return m_caption; }
  const UnitLabel label() const override;

  Label();
  Label(const std::string &caption, const std::string &label);
  void setLabel(const std::string &cpt, const UnitLabel &lbl = UnitLabel(""));
  Unit *clone() const override;

private:
  /// Caption
  std::string m_caption;
  /// Label
  UnitLabel m_label;
};

//=================================================================================================
/// Time of flight in microseconds
class MANTID_KERNEL_DLL TOF : public Unit {
public:
  const std::string unitID() const override; ///< "TOF"
  const std::string caption() const override { return "Time-of-flight"; }
  const UnitLabel label() const override;

  TOF();
  void init() override;
  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  Unit *clone() const override;
  ///@return -DBL_MAX as ToF convertible to TOF for in any time range
  double conversionTOFMin() const override;
  ///@return DBL_MAX as ToF convertible  to TOF for in any time range
  double conversionTOFMax() const override;
};

//=================================================================================================
/// Wavelength in Angstrom
class MANTID_KERNEL_DLL Wavelength : public Unit {
public:
  const std::string unitID() const override; ///< "Wavelength"
  const std::string caption() const override { return "Wavelength"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;

  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  Wavelength();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double efixed;
  double sfpTo;      ///< Extra correction factor in to conversion
  double factorTo;   ///< Constant factor for to conversion
  double sfpFrom;    ///< Extra correction factor in from conversion
  double factorFrom; ///< Constant factor for from conversion
  bool do_sfpFrom;   ///< Apply the sfpFrom value
};

//=================================================================================================
/// Energy in milli-electronvolts
class MANTID_KERNEL_DLL Energy : public Unit {
public:
  const std::string unitID() const override; ///< "Energy"
  const std::string caption() const override { return "Energy"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;

  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  Energy();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Absolute energy in units of wavenumber (cm^-1)
class MANTID_KERNEL_DLL Energy_inWavenumber : public Unit {
public:
  const std::string unitID() const override; ///< "Energy_inWavenumber"
  const std::string caption() const override { return "Energy"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  Energy_inWavenumber();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

MANTID_KERNEL_DLL double tofToDSpacingFactor(const double l1, const double l2, const double twoTheta,
                                             const double offset);

MANTID_KERNEL_DLL double calculateDIFCCorrection(const double l1, const double l2, const double twotheta,
                                                 const double offset, const double binWidth);

//=================================================================================================
/// d-Spacing in Angstrom
class MANTID_KERNEL_DLL dSpacing : public Unit {
public:
  const std::string unitID() const override; ///< "dSpacing"
  const std::string caption() const override { return "d-Spacing"; }
  const UnitLabel label() const override;
  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;
  double calcTofMin(const double difc, const double difa, const double tzero, const double tofmin = 0.);
  double calcTofMax(const double difc, const double difa, const double tzero, const double tofmax = 0.);

  /// Constructor
  dSpacing();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  std::string toDSpacingError;
  double difa;
  double difc;
  double tzero;
};

//=================================================================================================
/// d-SpacingPerpendicular in Angstrom
class MANTID_KERNEL_DLL dSpacingPerpendicular : public Unit {
public:
  const std::string unitID() const override; ///< "dSpacingPerpendicular"
  const std::string caption() const override { return "d-SpacingPerpendicular"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  dSpacingPerpendicular();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double twoTheta;
  double factorTo;   ///< Constant factor for to conversion
  double sfpTo;      ///< Extra correction factor in to conversion
  double factorFrom; ///< Constant factor for from conversion
  double sfpFrom;    ///< Extra correction factor in to conversion
};

//=================================================================================================
/// Momentum Transfer in Angstrom^-1
class MANTID_KERNEL_DLL MomentumTransfer : public Unit {
public:
  const std::string unitID() const override; ///< "MomentumTransfer"
  const std::string caption() const override { return "q"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;
  /// Constructor
  MomentumTransfer();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double difc;
};

//=================================================================================================
/// Momentum transfer squared in Angstrom^-2
class MANTID_KERNEL_DLL QSquared : public MomentumTransfer {
public:
  const std::string unitID() const override; ///< "QSquared"
  const std::string caption() const override { return "Q2"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  QSquared();
};

//=================================================================================================
/// Energy transfer in milli-electronvolts
class MANTID_KERNEL_DLL DeltaE : public Unit {
public:
  const std::string unitID() const override; ///< "DeltaE"
  const std::string caption() const override { return "Energy transfer"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;

  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  DeltaE();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double efixed;
  double factorTo;    ///< Constant factor for to conversion
  double factorFrom;  ///< Constant factor for from conversion
  double t_other;     ///< Energy mode dependent factor in to conversion
  double t_otherFrom; ///< Energy mode dependent factor in from conversion
  double unitScaling; ///< Apply unit scaling to energy value
};

//=================================================================================================
/// Energy transfer in units of wavenumber (cm^-1)
class MANTID_KERNEL_DLL DeltaE_inWavenumber : public DeltaE {
public:
  const std::string unitID() const override; ///< "DeltaE_inWavenumber"
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;
  /// Constructor
  DeltaE_inWavenumber();
};

//=================================================================================================
/// Energy transfer in units of frequency (MHz)
class MANTID_KERNEL_DLL DeltaE_inFrequency : public DeltaE {
public:
  const std::string unitID() const override; ///< "DeltaE_inFrequency"
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;
  /// Constructor
  DeltaE_inFrequency();
};

//=================================================================================================
/// Momentum in Angstrom^-1
class MANTID_KERNEL_DLL Momentum : public Unit {
public:
  const std::string unitID() const override; ///< "Momentum"
  const std::string caption() const override { return "Momentum"; }
  const UnitLabel label() const override;

  double singleToTOF(const double ki) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  Momentum();

protected:
  void validateUnitParams(const int emode, const UnitParametersMap &params) override;
  double efixed;
  double sfpTo;      ///< Extra correction factor in to conversion
  double factorTo;   ///< Constant factor for to conversion
  double sfpFrom;    ///< Extra correction factor in from conversion
  double factorFrom; ///< Constant factor for from conversion
  bool do_sfpFrom;   ///< Apply the sfpFrom value
};

//=================================================================================================
/// SpinEchoLength in nm
class MANTID_KERNEL_DLL SpinEchoLength : public Wavelength {
public:
  const std::string unitID() const override; ///< "SpinEchoLength"
  const std::string caption() const override { return "Spin Echo Length"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  SpinEchoLength();

private:
  double efixed;
};

//=================================================================================================
/// SpinEchoTime in ns
class MANTID_KERNEL_DLL SpinEchoTime : public Wavelength {
public:
  const std::string unitID() const override; ///< "SpinEchoTime"
  const std::string caption() const override { return "Spin Echo Time"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  SpinEchoTime();

private:
  double efixed;
};

//=================================================================================================
/// Time In Second
class MANTID_KERNEL_DLL Time : public Unit {
public:
  const std::string unitID() const override; ///< "Time"
  const std::string caption() const override { return "t"; }
  const UnitLabel label() const override;

  bool isConvertible() const override { return false; }
  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  double conversionTOFMax() const override;
  double conversionTOFMin() const override;
  void init() override;
  Unit *clone() const override;

  /// Constructor
  Time();

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Degrees that has degrees as unit and "Scattering angle" as title
class MANTID_KERNEL_DLL Degrees : public Empty {
public:
  Degrees();
  const std::string unitID() const override; /// < Degrees
  const std::string caption() const override { return "Scattering angle"; }
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

private:
  UnitLabel m_label;
};

//=================================================================================================
/// Phi that has degrees as unit and "Phi" as title
class MANTID_KERNEL_DLL Phi : public Degrees {
  const std::string unitID() const override;
  const std::string caption() const override { return "Phi"; }
  Unit *clone() const override { return new Phi(*this); }
};

//=================================================================================================
/// Temperature in kelvin
class MANTID_KERNEL_DLL Temperature : public Empty {
public:
  Temperature();
  const std::string unitID() const override; ///< "Temperature"
  const std::string caption() const override { return "Temperature"; }
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

private:
  UnitLabel m_label;
};

//=================================================================================================
/// Atomic Distance in Angstroms
class MANTID_KERNEL_DLL AtomicDistance : public Empty {
public:
  AtomicDistance();
  const std::string unitID() const override; ///< "AtomicDistance"
  const std::string caption() const override { return "Atomic Distance"; }
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

private:
  UnitLabel m_label;
};

//=================================================================================================

MANTID_KERNEL_DLL double timeConversionValue(const std::string &input_unit, const std::string &output_unit);

template <typename T>
void timeConversionVector(std::vector<T> &vec, const std::string &input_unit, const std::string &output_unit) {
  double factor = timeConversionValue(std::move(input_unit), std::move(output_unit));
  if (factor != 1.0)
    std::transform(vec.begin(), vec.end(), vec.begin(), [factor](T x) -> T { return x * static_cast<T>(factor); });
}

} // namespace Units

} // namespace Kernel
} // namespace Mantid
