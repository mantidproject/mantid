#ifndef MANTID_KERNEL_UNIT_H_
#define MANTID_KERNEL_UNIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include <string>
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** The base units (abstract) class. All concrete units should inherit from
    this class and provide implementations of the unitCode(), caption(),
    label(), toTOF() and fromTOF() methods. They also need to register into
    the UnitFactory via the macro DECLARE_UNIT(classname).

    @author Russell Taylor, Tessella Support Services plc
    @date 25/02/2008
    
    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
  /// The unique code identifying the unit
  virtual const int unitCode() const = 0;
  /// The full name of the unit
  virtual const std::string caption() const = 0;
  /// A label for the unit to be printed on axes
  virtual const std::string label() const = 0;
  
  // Methods dealing with instance-level description
  const std::string& description() const;
  void setDescription(const std::string& value);
  
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta Not currently used
   *  @param emode    Not currently used
   *  @param efixed   Not currently used
   *  @param delta    Not currently used
   */
  virtual void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const = 0;

  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta Not currently used
   *  @param emode    Not currently used
   *  @param efixed   Not currently used
   *  @param delta    Not currently used
   */
  virtual void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const = 0;
  
  /// (Empty) Constructor
  Unit() {}
  /// Virtual destructor
	virtual ~Unit() {}
	
private:
  /// The optional description that can be attached to an instance of a concrete unit
  std::string m_description;
};



//----------------------------------------------------------------------
// Now the concrete units classes
//----------------------------------------------------------------------

/// The namespace for concrete units classes
namespace Units
{

class DLLExport TOF : public Unit
{
public:
  const int unitCode() const { return 1; }
  const std::string caption() const { return "Time-of-flight"; }
  const std::string label() const {return "microsecond"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  TOF() : Unit() {}
  /// Destructor
  ~TOF() {}
};

class DLLExport Wavelength : public Unit
{
public:
  const int unitCode() const { return 2; }
  const std::string caption() const { return "Wavelength"; }
  const std::string label() const {return "Angstrom"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  Wavelength() : Unit() {}
  /// Destructor
  ~Wavelength() {}
};

class DLLExport Energy : public Unit
{
public:
  const int unitCode() const { return 3; }
  const std::string caption() const { return "Energy"; }
  const std::string label() const {return "MeV"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  Energy() : Unit() {}
  /// Destructor
  ~Energy() {}
};

class DLLExport dSpacing : public Unit
{
public:
  const int unitCode() const { return 4; }
  const std::string caption() const { return "d-Spacing"; }
  const std::string label() const {return "Angstrom"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  dSpacing() : Unit() {}
  /// Destructor
  ~dSpacing() {}
};

class DLLExport MomentumTransfer : public Unit
{
public:
  const int unitCode() const { return 5; }
  const std::string caption() const { return "q"; }
  const std::string label() const {return "1/Angstrom"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  MomentumTransfer() : Unit() {}
  /// Destructor
  ~MomentumTransfer() {}
};

class DLLExport QSquared : public Unit
{
public:
  const int unitCode() const { return 6; }
  const std::string caption() const { return "Q2"; }
  const std::string label() const {return "Angstrom^-2"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  QSquared() : Unit() {}
  /// Destructor
  ~QSquared() {}
};

class DLLExport DeltaE : public Unit
{
public:
  const int unitCode() const { return 7; }
  const std::string caption() const { return "Energy transfer"; }
  const std::string label() const {return "meV"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2, 
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
  
  /// Constructor
  DeltaE() : Unit() {}
  /// Destructor
  ~DeltaE() {}
};

class DLLExport DeltaE_inWavenumber : public Unit
{
public:
  const int unitCode() const { return 8; }
  const std::string caption() const { return "Energy transfer"; }
  const std::string label() const {return "1/cm"; }
  
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const;
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
