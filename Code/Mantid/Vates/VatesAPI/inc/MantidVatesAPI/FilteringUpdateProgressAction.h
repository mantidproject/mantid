#ifndef PARAVIEWPROGRESSACTION_H_
#define PARAVIEWPROGRESSACTION_H_

#include "MantidKernel/System.h"
#include "MantidVatesAPI/ProgressAction.h"

/** Adapter for action specific to ParaView RebinningCutter filter. Handles progress actions raised by underlying Mantid Algorithms.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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


namespace Mantid
{
namespace VATES
{

/// Template argument is the exact filter/source/reader providing the public UpdateAlgorithmProgress method.
template<typename Filter>
class DLLExport FilterUpdateProgressAction : public ProgressAction
{

public:

  FilterUpdateProgressAction(Filter* filter, const std::string& message) : m_filter(filter), m_message(message)
  {
  }

  virtual void eventRaised(double progress)
  {
    m_filter->updateAlgorithmProgress(progress, m_message);
  }

  ~FilterUpdateProgressAction()
  {
  }

private:

  FilterUpdateProgressAction& operator=(FilterUpdateProgressAction&);

  FilterUpdateProgressAction(FilterUpdateProgressAction&);

  Filter* m_filter;

  /// Message associated with the progress action
  std::string m_message;
};

}
}

#endif /* PARAVIEWPROGRESSACTION_H_ */
