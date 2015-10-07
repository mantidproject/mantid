#ifndef MANTID_KERNEL_UNIT_H_
#define MANTID_KERNEL_UNIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitLabel.h"
#include <map>
#include <vector>
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

namespace Mantid {
namespace Kernel {

/** The base units (abstract) class. All concrete units should inherit from
    this class and provide implementations of the caption(), label(),
    toTOF() and fromTOF() methods. They also need to declare (but NOT define)
    the unitID() method and register into the UnitFactory via the macro
   DECLARE_UNIT(classname).

    @author Russell Taylor, Tessella Support Services plc
    @date 25/02/2008

    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL Unit {
public:
  /// (Empty) Constructor
  Unit();
  /// Virtual destructor
  virtual ~Unit();
  /// Copy Constructor
  Unit(const Unit &other);
  /// Copy assignment operator
  Unit &operator=(const Unit &rhs);

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
   *  @param l1 ::       The source-sample distance (in metres)
   *  @param l2 ::       The sample-detector distance (in metres)
   *  @param twoTheta :: The scattering angle (in radians)
   *  @param emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
   * meV)
   *  @param delta ::    Not currently used
   */
  void toTOF(std::vector<double> &xdata, std::vector<double> &ydata,
             const double &l1, const double &l2, const double &twoTheta,
             const int &emode, const double &efixed, const double &delta);

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
   *  @param l1 ::       The source-sample distance (in metres)
   *  @param l2 ::       The sample-detector distance (in metres)
   *  @param twoTheta :: The scattering angle (in radians)
   *  @param emode ::    The energy mode (0=elastic, 1=direct geometry,
   * 2=indirect geometry)
   *  @param efixed ::   Value of fixed energy: EI (emode=1) or EF (emode=2) (in
   * meV)
   *  @param delta ::    Not currently used
   */
  void fromTOF(std::vector<double> &xdata, std::vector<double> &ydata,
               const double &l1, const double &l2, const double &twoTheta,
               const int &emode, const double &efixed, const double &delta);

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
  typedef std::pair<double, double> ConstantAndPower;
  /// Lists, for a given starting unit, the units to which a 'quick conversion'
  /// can be made
  typedef std::map<std::string, ConstantAndPower> UnitConversions;
  /// The possible 'quick conversions' are held in a map with the starting unit
  /// as the key
  typedef std::map<std::string, UnitConversions> ConversionsMap;
  /// The table of possible 'quick conversions'
  static ConversionsMap s_conversionFactors;
};

/// Shared pointer to the Unit base class
typedef boost::shared_ptr<Unit> Unit_sptr;
/// Shared pointer to the Unit base class (const version)
typedef boost::shared_ptr<const Unit> Unit_const_sptr;

//----------------------------------------------------------------------
// Now the concrete units classes
//----------------------------------------------------------------------

/// The namespace for concrete units classes
namespace Units {

//=================================================================================================
/// Empty unit
class MANTID_KERNEL_DLL Empty : public Unit {
public:
  const std::string unitID() const; ///< "Empty"
  const std::string caption() const { return ""; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;

  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  Empty() : Unit() {}
  /// Destructor
  ~Empty() {}
};

//=================================================================================================
/// Label unit
class MANTID_KERNEL_DLL Label : public Empty {
public:
  const std::string unitID() const; ///< "Label"
  const std::string caption() const { return m_caption; }
  const UnitLabel label() const;

  Label();
  Label(const std::string &caption, const std::string &label);
  void setLabel(const std::string &cpt, const UnitLabel &lbl = UnitLabel(""));
  virtual Unit *clone() const;

  /// Destructor
  ~Label() {}

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
  const std::string unitID() const; ///< "TOF"
  const std::string caption() const { return "Time-of-flight"; }
  const UnitLabel label() const;

  TOF();
  virtual void init();
  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual Unit *clone() const;
  ///@return -DBL_MAX as ToF convertible to TOF for in any time range
  virtual double conversionTOFMin() const;
  ///@return DBL_MAX as ToF convertible  to TOF for in any time range
  virtual double conversionTOFMax() const;
};

//=================================================================================================
/// Wavelength in Angstrom
class MANTID_KERNEL_DLL Wavelength : public Unit {
public:
  const std::string unitID() const; ///< "Wavelength"
  const std::string caption() const { return "Wavelength"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;

  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  Wavelength();
  /// Destructor
  ~Wavelength() {}

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
  const std::string unitID() const; ///< "Energy"
  const std::string caption() const { return "Energy"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;

  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  Energy();
  /// Destructor
  ~Energy() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Absolute energy in units of wavenumber (cm^-1)
class MANTID_KERNEL_DLL Energy_inWavenumber : public Unit {
public:
  const std::string unitID() const; ///< "Energy_inWavenumber"
  const std::string caption() const { return "Energy"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  Energy_inWavenumber();
  /// Destructor
  ~Energy_inWavenumber() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// d-Spacing in Angstrom
class MANTID_KERNEL_DLL dSpacing : public Unit {
public:
  const std::string unitID() const; ///< "dSpacing"
  const std::string caption() const { return "d-Spacing"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  dSpacing();
  /// Destructor
  ~dSpacing() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Momentum Transfer in Angstrom^-1
class MANTID_KERNEL_DLL MomentumTransfer : public Unit {
public:
  const std::string unitID() const; ///< "MomentumTransfer"
  const std::string caption() const { return "q"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;
  /// Constructor
  MomentumTransfer();
  /// Destructor
  ~MomentumTransfer() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Momentum transfer squared in Angstrom^-2
class MANTID_KERNEL_DLL QSquared : public Unit {
public:
  const std::string unitID() const; ///< "QSquared"
  const std::string caption() const { return "Q2"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  QSquared();
  /// Destructor
  ~QSquared() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Energy transfer in milli-electronvolts
class MANTID_KERNEL_DLL DeltaE : public Unit {
public:
  virtual const std::string unitID() const; ///< "DeltaE"
  virtual const std::string caption() const { return "Energy transfer"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;

  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  DeltaE();
  /// Destructor
  ~DeltaE() {}

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
  const std::string unitID() const; ///< "DeltaE_inWavenumber"
  const std::string caption() const { return "Energy transfer"; }
  const UnitLabel label() const;

  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;
  /// Constructor
  DeltaE_inWavenumber();
  /// Destructor
  ~DeltaE_inWavenumber() {}
};

//=================================================================================================
/// Momentum in Angstrom^-1
class MANTID_KERNEL_DLL Momentum : public Unit {
public:
  const std::string unitID() const; ///< "Momentum"
  const std::string caption() const { return "Momentum"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  Momentum();
  /// Destructor
  ~Momentum() {}

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
  const std::string unitID() const; ///< "SpinEchoLength"
  const std::string caption() const { return "Spin Echo Length"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  SpinEchoLength();
  /// Destructor
  ~SpinEchoLength() {}
};

//=================================================================================================
/// SpinEchoTime in ns
class MANTID_KERNEL_DLL SpinEchoTime : public Wavelength {
public:
  const std::string unitID() const; ///< "SpinEchoTime"
  const std::string caption() const { return "Spin Echo Time"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual void init();
  virtual Unit *clone() const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

  /// Constructor
  SpinEchoTime();
  /// Destructor
  ~SpinEchoTime() {}
};

//=================================================================================================
/// Time In Second
class MANTID_KERNEL_DLL Time : public Unit {
public:
  const std::string unitID() const; ///< "Time"
  const std::string caption() const { return "t"; }
  const UnitLabel label() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual double conversionTOFMax() const;
  virtual double conversionTOFMin() const;
  virtual void init();
  virtual Unit *clone() const;

  /// Constructor
  Time();
  /// Destructor
  ~Time() {}

protected:
  double factorTo;   ///< Constant factor for to conversion
  double factorFrom; ///< Constant factor for from conversion
};

//=================================================================================================
/// Degrees that has degrees as unit at "Scattering angle" as title
class MANTID_KERNEL_DLL Degrees : public Empty {
public:
  Degrees();
  const std::string unitID() const; /// < Degrees
  const std::string caption() const { return "Scattering angle"; }
  const UnitLabel label() const;

  virtual void init();
  virtual Unit *clone() const;

  virtual double singleToTOF(const double x) const;
  virtual double singleFromTOF(const double tof) const;
  virtual double conversionTOFMin() const;
  virtual double conversionTOFMax() const;

private:
  UnitLabel m_label;
};

//=================================================================================================

/// Phi that has degrees as unit at "Phi" as title
class MANTID_KERNEL_DLL Phi : public Degrees {
  virtual const std::string caption() const { return "Phi"; }
  virtual Unit *clone() const { return new Phi(*this); }
};

//=================================================================================================

} // namespace Units

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_UNIT_H_*/
