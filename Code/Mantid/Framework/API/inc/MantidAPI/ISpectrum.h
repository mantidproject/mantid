#ifndef MANTID_API_ISPECTRUM_H_
#define MANTID_API_ISPECTRUM_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/cow_ptr.h"
#include <set>

namespace Mantid {
namespace API {

/** A "spectrum" is an object that holds the data for a particular spectrum,
 * in particular:
 *  - The X/Y/E arrays
 *  - The spectrum number
 *  - A list of detector ids associated with it.
 *
 * This is an interface that can be used for both Workspace2D's Spectrum
 objects,
 * and EventWorkspace's EventList objects

  @author Janik Zikovsky
  @date 2011-07-01

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ISpectrum {
public:
  ISpectrum();
  ISpectrum(const specid_t specNo);
  ISpectrum(const ISpectrum &other);
  virtual ~ISpectrum();

  void copyInfoFrom(const ISpectrum &other);

  virtual void setX(const MantidVec &X);
  virtual void setDx(const MantidVec &Dx);

  virtual void setX(const MantidVecPtr &X);
  virtual void setDx(const MantidVecPtr &Dx);

  virtual void setX(const MantidVecPtr::ptr_type &X);
  virtual void setDx(const MantidVecPtr::ptr_type &Dx);

  virtual MantidVec &dataX();
  virtual MantidVec &dataDx();

  virtual const MantidVec &dataX() const;
  virtual const MantidVec &dataDx() const;

  virtual const MantidVec &readX() const;
  virtual const MantidVec &readDx() const;

  virtual MantidVecPtr ptrX() const;
  virtual MantidVecPtr ptrDx() const;

  virtual void setData(const MantidVec &Y) = 0;
  virtual void setData(const MantidVec &Y, const MantidVec &E) = 0;

  virtual void setData(const MantidVecPtr &Y) = 0;
  virtual void setData(const MantidVecPtr &Y, const MantidVecPtr &E) = 0;

  virtual void setData(const MantidVecPtr::ptr_type &Y) = 0;
  virtual void setData(const MantidVecPtr::ptr_type &Y,
                       const MantidVecPtr::ptr_type &E) = 0;

  virtual void clearData() = 0;

  virtual MantidVec &dataY() = 0;
  virtual MantidVec &dataE() = 0;

  virtual const MantidVec &dataY() const = 0;
  virtual const MantidVec &dataE() const = 0;
  virtual const MantidVec &readY() const;
  virtual const MantidVec &readE() const;

  virtual size_t getMemorySize() const = 0;

  virtual std::pair<double, double> getXDataRange() const;
  // ---------------------------------------------------------
  void addDetectorID(const detid_t detID);
  void addDetectorIDs(const std::set<detid_t> &detIDs);
  void addDetectorIDs(const std::vector<detid_t> &detIDs);
  void setDetectorID(const detid_t detID);
  void setDetectorIDs(const std::set<detid_t> &detIDs);
  void setDetectorIDs(std::set<detid_t> &&detIDs);

  bool hasDetectorID(const detid_t detID) const;
  std::set<detid_t> &getDetectorIDs();
  const std::set<detid_t> &getDetectorIDs() const;

  void clearDetectorIDs();

  // ---------------------------------------------------------
  specid_t getSpectrumNo() const;

  void setSpectrumNo(specid_t num);

  // ---------------------------------------------------------
  virtual void lockData() const;
  virtual void unlockData() const;

protected:
  /// The spectrum number of this spectrum
  specid_t m_specNo;

  /// Set of the detector IDs associated with this spectrum
  std::set<detid_t> detectorIDs;

  /// Copy-on-write pointer to the X data vector.
  MantidVecPtr refX;

  /// Copy-on-write pointer to the Dx (X error) vector.
  MantidVecPtr refDx;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ISPECTRUM_H_ */
