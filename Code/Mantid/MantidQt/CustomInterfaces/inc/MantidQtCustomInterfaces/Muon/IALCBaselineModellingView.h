#ifndef MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <QObject>
#include "qwt_data.h"

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
    typedef std::pair<double,double> SectionSelector;

    /// Function chosen to fit the data to
    /// @return Initialized function
    virtual IFunction_const_sptr function() const = 0;

    /// List of sections from the table
    virtual std::vector<Section> sections() const = 0;

  public slots:
    /// Performs any necessary initialization
    virtual void initialize() = 0;

    /**
     * Update displayed data curve
     * @param data :: New curve data
     */
    virtual void setDataCurve(const QwtData& data) = 0;

    /**
     * Update displayed corrected data curve
     * @param data :: New curve data
     */
    virtual void setCorrectedCurve(const QwtData& data) = 0;

    /**
     * Update displayed baseline curve
     * @param data :: New curve data
     */
    virtual void setBaselineCurve(const QwtData& data) = 0;

    /**
     * Update displayed function
     * @param func :: New function
     */
    virtual void setFunction(IFunction_const_sptr func) = 0;

    /**
     * Reset a list of sections displayed
     * @param sections :: New list of sections to display
     */
    virtual void setSections(const std::vector<Section>& sections) = 0;

    /**
     * Modify section values
     * @param index :: Index of the section to modify
     * @param min :: New section min value
     * @param max :: New section max value
     */
    virtual void updateSection(size_t index, double min, double max) = 0;

    /**
     * Reset a list of section selectors
     * @param selectors :: New list of selectors
     */
    virtual void setSectionSelectors(const std::vector<SectionSelector>& selectors) = 0;

    /**
     * Update values of a single section selector
     * @param index :: Index of the selector to update
     * @param min :: New min value
     * @param max :: New max value
     */
    virtual void updateSectionSelector(size_t index, double min, double max) = 0;

  signals:
    /// Fit requested
    void fitRequested();

    /// New section addition requested
    void addSectionRequested();

    /// Removal of section requested
    void removeSectionRequested(size_t index);

    /**
     * One of the sections in the table was modified
     * @param index :: Index of the modified section
     * @param min :: New min value
     * @param max :: New max value
     */
    void sectionModified(size_t index, double min, double max);

    /**
     * One of section selectors has been modified
     * @param index :: Index of modified selector
     * @param min :: New min value
     * @param max :: New max value
     */
    void sectionSelectorModified(size_t index, double min, double max);
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_ */
