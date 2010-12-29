#ifndef MANTID_KERNEL_UNIT_H_
#define MANTID_KERNEL_UNIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Kernel
{
/** The base units (abstract) class. All concrete units should inherit from
    this class and provide implementations of the caption(), label(),
    toTOF() and fromTOF() methods. They also need to declare (but NOT define)
    the unitID() method and register into the UnitFactory via the macro DECLARE_UNIT(classname).

    @author Russell Taylor, Tessella Support Services plc
    @date 25/02/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Unit
{
public:
  /// The name of the unit. For a concrete unit, this method's definition is in the DECLARE_UNIT
  /// macro and it will return the argument passed to that macro (which is the unit's key in the
  /// factory).
  /// @return The unit ID
  virtual const std::string unitID() const = 0;
  /// The full name of the unit
  /// @return The unit caption
  virtual const std::string caption() const = 0;
  /// A label for the unit to be printed on axes
  /// @return The unit label
  virtual const std::string label() const = 0;

  // Check whether the unit can be converted to another via a simple factor
  bool quickConversion(const Unit& destination, double& factor, double& power) const;
  bool quickConversion(std::string destUnitName, double& factor, double& power) const;

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  virtual void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const = 0;

  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  virtual void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const = 0;

  /// (Empty) Constructor
  Unit() {}
  /// Virtual destructor
  virtual ~Unit() {}

protected:
  // Add a 'quick conversion' for a unit pair
  void addConversion(std::string to, const double& factor, const double& power = 1.0) const;

private:
  /// A 'quick conversion' requires the constant by which to multiply the input and the power to which to raise it
  typedef std::pair< double, double > ConstantAndPower;
  /// Lists, for a given starting unit, the units to which a 'quick conversion' can be made
  typedef std::map< std::string, ConstantAndPower > UnitConversions;
  /// The possible 'quick conversions' are held in a map with the starting unit as the key
  typedef std::map< std::string, UnitConversions > ConversionsMap;
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
namespace Units
{

/// Empty unit
class DLLExport Empty : public Unit
{
public:
  const std::string unitID() const; ///< "Empty"
  const std::string caption() const { return ""; }
  const std::string label() const {return ""; }

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  Empty() : Unit() {}
  /// Destructor
  ~Empty() {}
};

/// Label unit
class DLLExport Label : public Empty
{
public:
  const std::string unitID() const; ///< "Label"
  const std::string caption() const { return m_caption; }
  const std::string label() const {return m_label; }

  Label();
  void setLabel(const std::string& cpt, const std::string& lbl = "");

  /// Destructor
  ~Label() {}
private:
  /// Caption
  std::string m_caption;
  /// Label
  std::string m_label;
};

/// Time of flight in microseconds
class DLLExport TOF : public Unit
{
public:
  const std::string unitID() const; ///< "TOF"
  const std::string caption() const { return "Time-of-flight"; }
  const std::string label() const {return "microsecond"; }
  
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  TOF() : Unit() {}
  /// Destructor
  ~TOF() {}
};

/// Wavelength in Angstrom
class DLLExport Wavelength : public Unit
{
public:
  const std::string unitID() const; ///< "Wavelength"
  const std::string caption() const { return "Wavelength"; }
  const std::string label() const {return "Angstrom"; }
  
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  Wavelength();
  /// Destructor
  ~Wavelength() {}
};

/// Energy in milli-electronvolts
class DLLExport Energy : public Unit
{
public:
  const std::string unitID() const; ///< "Energy"
  const std::string caption() const { return "Energy"; }
  const std::string label() const {return "meV"; }

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  Energy();
  /// Destructor
  ~Energy() {}
};

/// d-Spacing in Angstrom
class DLLExport dSpacing : public Unit
{
public:
  const std::string unitID() const; ///< "dSpacing"
  const std::string caption() const { return "d-Spacing"; }
  const std::string label() const {return "Angstrom"; }
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  dSpacing();
  /// Destructor
  ~dSpacing() {}
};

/// Momentum Transfer in Angstrom^-1
class DLLExport MomentumTransfer : public Unit
{
public:
  const std::string unitID() const; ///< "MomentumTransfer"
  const std::string caption() const { return "q"; }
  const std::string label() const {return "1/Angstrom"; }

  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  MomentumTransfer();
  /// Destructor
  ~MomentumTransfer() {}
};

/// Momentum transfer squared in Angstrom^-2
class DLLExport QSquared : public Unit
{
public:
  const std::string unitID() const; ///< "QSquared"
  const std::string caption() const { return "Q2"; }
  const std::string label() const {return "Angstrom^-2"; }
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  QSquared();
  /// Destructor
  ~QSquared() {}
};

/// Energy transfer in milli-electronvolts
class DLLExport DeltaE : public Unit
{
public:
  const std::string unitID() const; ///< "DeltaE"
  const std::string caption() const { return "Energy transfer"; }
  const std::string label() const {return "meV"; }
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  DeltaE() : Unit() {}
  /// Destructor
  ~DeltaE() {}
};

/// Energy transfer in units of wavenumber (cm^-1)
class DLLExport DeltaE_inWavenumber : public Unit
{
public:
  const std::string unitID() const; ///< "DeltaE_inWavenumber"
  const std::string caption() const { return "Energy transfer"; }
  const std::string label() const {return "1/cm"; }
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;

  /// Constructor
  DeltaE_inWavenumber() : Unit() {}
  /// Destructor
  ~DeltaE_inWavenumber() {}
};

} // namespace Units

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_UNIT_H_*/
