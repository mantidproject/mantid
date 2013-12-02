#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid
{
namespace Algorithms
{

  /** ReflectometryReductionOne : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ReflectometryReductionOne  : public API::DataProcessorAlgorithm
  {
  public:
    ReflectometryReductionOne();
    virtual ~ReflectometryReductionOne();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    typedef boost::tuple<double, double> MinMax;
    typedef boost::optional<double> OptionalDouble;
    typedef boost::optional<Mantid::API::MatrixWorkspace_sptr> OptionalMatrixWorkspace_sptr;
    typedef std::vector<int> WorkspaceIndexList;
    typedef boost::optional< std::vector< int > > OptionalWorkspaceIndexes;
    bool isPropertyDefault(const std::string& propertyName) const;
    WorkspaceIndexList getWorkspaceIndexList();
    void fetchOptionalLowerUpperPropertyValue(const std::string& propertyName, bool isPointDetector, OptionalWorkspaceIndexes& optionalUpperLower);
    MinMax getMinMax(const std::string& minProperty, const std::string& maxProperty);
    void getTransmissionRunInfo(OptionalMatrixWorkspace_sptr firstTransmissionRun, OptionalMatrixWorkspace_sptr secondTransmissionRun, OptionalDouble& stitchingStartQ, OptionalDouble& stitchingDeltaQ, OptionalDouble& stitchingEndQ);
    virtual void initDocs();
    void init();
    void exec();


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE_H_ */
