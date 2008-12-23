#ifndef MANTID_KERNEL_ParInstrument_H_
#define MANTID_KERNEL_ParInstrument_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidGeometry/ParCompAssembly.h"
#include "MantidGeometry/ParObjComponent.h"
#include "MantidGeometry/Detector.h"
#include "MantidAPI/Instrument.h"
#include <string>
#include <ostream>

namespace Mantid
{
namespace API
{
/** @class ParParInstrument ParParInstrument.h

 	ParParInstrument Class.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
//class Instrument;

class DLLExport ParInstrument : public Geometry::ParCompAssembly, public IInstrument
{
public:
  ///String description of the type of component
  std::string type() const { return "ParInstrument"; }

  //ParInstrument( Geometry::ParameterMap* map){}
  ParInstrument(const boost::shared_ptr<Instrument> instr,const boost::shared_ptr<Geometry::ParameterMap> map);
  ///Virtual destructor
  virtual ~ParInstrument() {}

  virtual boost::shared_ptr<Geometry::IObjComponent> getSource() const;
  virtual boost::shared_ptr<Geometry::IObjComponent>  getSample() const;
  virtual boost::shared_ptr<Geometry::IDetector> getDetector(const int &detector_id) const;
  virtual const double detectorTwoTheta(const boost::shared_ptr<Geometry::IDetector>) const;

  boost::shared_ptr<Instrument> baseInstrument()const{return m_instr;}
  boost::shared_ptr<Geometry::ParameterMap> getParameterMap(){return m_parmap;}


  /// return reference to detector cache 
  std::map<int,  boost::shared_ptr<Geometry::IDetector> > getDetectors();

  /// Get pointers to plottable components
  virtual std::vector< boost::shared_ptr<Geometry::IObjComponent> > getPlottable()const;

  std::string getName()const{return Geometry::ParCompAssembly::getName();}

private:
  /// Private copy assignment operator
  ParInstrument& operator=(const ParInstrument&);
  /// Private copy constructor
  ParInstrument(const ParInstrument&);
  boost::shared_ptr<Instrument> m_instr;
  boost::shared_ptr<Geometry::ParameterMap> m_parmap;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;

};

} // namespace API
} //Namespace Mantid
#endif /*MANTID_KERNEL_ParInstrument_H_*/
