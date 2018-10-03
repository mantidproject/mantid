// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_UNIT_H_
#define MANTID_KERNEL_UNIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/UnitLabel.h"
#include <vector>
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include "tbb/concurrent_unordered_map.h"

namespace Mantid {
namespace Kernel {

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

  // Equality operators based on the value returned by unitID();
  bool operator==(const Unit &u) const;
  bool operator!=(const Unit &u) const;

  // Check whether the unit can be converted to another via a simple factor
  bool quickConversion(const Unit &destination, double &factor,
                       double &power) const;
  bool quickConversion(std::string destUnitName, double &factor,
                       double &power) const;

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata ::    The array of X data to be converted
   *  @param ydata ::    Not currently used (ConvertUnits passes an empty
   * vector)
   *  @param _l1 ::       The source-sample distance (in metres)
   *  @param _l2 ::       The sample-detector distance (in metres)
   *  @param _twoTheta :: The scattering angle (in radians)
   *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param _efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2)
   * (in
   * meV)
   *  @param _delta ::    Not currently used
   */
  void toTOF(std::vector<double> &xdata, std::vector<double> &ydata,
             const double &_l1, const double &_l2, const double &_twoTheta,
             const int &_emode, const double &_efixed, const double &_delta);

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xvalue ::   A single X-value to convert
   *  @param l1 ::       The source-sample distance (in metres)
   *  @param l2 ::       The sample-detector distance (in metres)
   *  @param twoTheta :: The scattering angle (in radians)
   *  @param emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
   * meV)
   *  @param delta ::    Not currently used
   *  @return the value in TOF units.
   */
  double convertSingleToTOF(const double xvalue, const double &l1,
                            const double &l2, const double &twoTheta,
                            const int &emode, const double &efixed,
                            const double &delta);

  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata ::    The array of X data to be converted
   *  @param ydata ::    Not currently used (ConvertUnits passes an empty
   * vector)
   *  @param _l1 ::       The source-sample distance (in metres)
   *  @param _l2 ::       The sample-detector distance (in metres)
   *  @param _twoTheta :: The scattering angle (in radians)
   *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param _efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2)
   * (in
   * meV)
   *  @param _delta ::    Not currently used
   */
  void fromTOF(std::vector<double> &xdata, std::vector<double> &ydata,
               const double &_l1, const double &_l2, const double &_twoTheta,
               const int &_emode, const double &_efixed, const double &_delta);

  /** Convert from the time-of-flight to the concrete unit. TOF is in
   * microseconds.
   *  @param xvalue ::   A single X-value to convert
   *  @param l1 ::       The source-sample distance (in metres)
   *  @param l2 ::       The sample-detector distance (in metres)
   *  @param twoTheta :: The scattering angle (in radians)
   *  @param emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
   * meV)
   *  @param delta ::    Not currently used
   *  @return the value in these units.
   */
  double convertSingleFromTOF(const double xvalue, const double &l1,
                              const double &l2, const double &twoTheta,
                              const int &emode, const double &efixed,
                              const double &delta);

  /** Initialize the unit to perform conversion using singleToTof() and
   *singleFromTof()
   *
   *  @param _l1 ::       The source-sample distance (in metres)
   *  @param _l2 ::       The sample-detector distance (in metres)
   *  @param _twoTheta :: The scattering angle (in radians)
   *  @param _emode ::    The energy mode (0=elastic, 1=direct geometry,
   *2=indirect geometry)
   *  @param _efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2)
   *(in meV)
   *  @param _delta ::    Not currently used
   */
  void initialize(const double &_l1, const double &_l2, const double &_twoTheta,
                  const int &_emode, const double &_efixed,
                  const double &_delta);

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
  void addConversion(std::string to, const double &factor,
                     const double &power = 1.0) const;

  /// Removes all registered 'quick conversions'
  void clearConversions() const;

  /// The unit values have been initialized
  bool initialized;
  /// l1 ::       The source-sample distance (in metres)
  double l1;
  /// l2 ::       The sample-detector distance (in metres)
  double l2;
  /// twoTheta :: The scattering angle (in radians)
  double twoTheta;
  /// emode ::    The energy mode (0=elastic, 1=direct geometry, 2=indirect
  /// geometry)
  int emode;
  /// efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
  double efixed;
  /// delta ::    Not currently used
  double delta;

private:
  /// A 'quick conversion' requires the constant by which to multiply the input
  /// and the power to which to raise it
  using ConstantAndPower = std::pair<double, double>;
  /// Lists, for a given starting unit, the units to which a 'quick conversion'
  /// can be made
  using UnitConversions =
      tbb::concurrent_unordered_map<std::string, ConstantAndPower>;
  /// The possible 'quick conversions' are held in a map with the starting unit
  /// as the key
  using ConversionsMap =
      tbb::concurrent_unordered_map<std::string, UnitConversions>;
  /// The table of possible 'quick conversions'
  static ConversionsMap s_conversionFactors;
};

/// Shared pointer to the Unit base class
using Unit_sptr = boost::shared_ptr<Unit>;
/// Shared pointer to the Unit base class (const version)
using Unit_const_sptr = boost::shared_ptr<const Unit>;

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
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

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

  /// Constructor
  dSpacing();

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// d-SpacingPerpendicular in Angstrom
class MANTID_KERNEL_DLL dSpacingPerpendicular : public Unit {
public:
  const std::string unitID() const override; ///< "dSpacingPerpendicular"
  const std::string caption() const override {
    return "d-SpacingPerpendicular";
  }
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
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Momentum transfer squared in Angstrom^-2
class MANTID_KERNEL_DLL QSquared : public Unit {
public:
  const std::string unitID() const override; ///< "QSquared"
  const std::string caption() const override { return "Q2"; }
  const UnitLabel label() const override;

  double singleToTOF(const double x) const override;
  double singleFromTOF(const double tof) const override;
  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;

  /// Constructor
  QSquared();

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
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
  const std::string caption() const override { return "Energy transfer"; }
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;
  /// Constructor
  DeltaE_inWavenumber();
};

//=================================================================================================
/// Energy transfer in units of frequency (GHz)
class MANTID_KERNEL_DLL DeltaE_inFrequency : public DeltaE {
public:
  const std::string unitID() const override; ///< "DeltaE_inFrequency"
  const std::string caption() const override { return "Energy transfer"; }
  const UnitLabel label() const override;

  void init() override;
  Unit *clone() const override;
  double conversionTOFMin() const override;
  double conversionTOFMax() const override;
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
};

//=================================================================================================
/// Time In Second
class MANTID_KERNEL_DLL Time : public Unit {
public:
  const std::string unitID() const override; ///< "Time"
  const std::string caption() const override { return "t"; }
  const UnitLabel label() const override;

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
/// Degrees that has degrees as unit at "Scattering angle" as title
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

/// Phi that has degrees as unit at "Phi" as title
class MANTID_KERNEL_DLL Phi : public Degrees {
  const std::string caption() const override { return "Phi"; }
  Unit *clone() const override { return new Phi(*this); }
};

//=================================================================================================

} // namespace Units

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_UNIT_H_*/
