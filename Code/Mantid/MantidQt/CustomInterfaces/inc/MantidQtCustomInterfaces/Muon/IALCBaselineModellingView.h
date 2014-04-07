#ifndef MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <QObject>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  /** IALCBaselineModellingView : Interface for ALC Baseline Modelling view step
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCBaselineModellingView : public QObject
  {
    Q_OBJECT

  public:
    typedef std::pair<double,double> Section;

    /// Function chosen to fit the data to
    /// @return Initialized function
    virtual IFunction_const_sptr function() const = 0;

  public slots:
    /// Performs any necessary initialization
    virtual void initialize() = 0;

    /// @param data :: New data to display
    virtual void setData(MatrixWorkspace_const_sptr data) = 0;

    /// @param data :: New corrected data to display
    virtual void setCorrectedData(MatrixWorkspace_const_sptr data) = 0;

    /// @param func :: New function to display
    virtual void setFunction(IFunction_const_sptr func) = 0;

    /// @param sections :: New list of sections to display
    virtual void setSections(const std::vector<Section>& sections) = 0;

  signals:
    /// Request to fit the data according to the function and sections
    void fit();

    /// Request to add a new section
    /// @param section :: Section to add
    void addSection(Section section);
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_ */
