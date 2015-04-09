#ifndef MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidAPI/IFunction.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

#include <QObject>
#include "qwt_data.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  /** IALCBaselineModellingView : Interface for ALC Baseline Modelling view step
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    typedef std::pair<QString,QString> SectionRow;
    typedef std::pair<double,double> SectionSelector;

    /// Function chosen to fit the data to
    /// @return Function string, or empty string if nothing chosen
    virtual QString function() const = 0;

    /**
     * @param row :: Row number
     * @return Row values from the section table
     */
    virtual SectionRow sectionRow(int row) const = 0;

    /**
     * @param index :: Section selector index
     * @return Section selector values
     */
    virtual SectionSelector sectionSelector(int index) const = 0;

    /**
     * @return Number of rows in section table
     */
    virtual int noOfSectionRows() const = 0;

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
    virtual void setFunction(Mantid::API::IFunction_const_sptr func) = 0;

    /**
     * Resize sections table
     * @param rows :: New number of rows
     */
    virtual void setNoOfSectionRows(int rows) = 0;

    /**
     * Updates the row values in the table
     * @param row :: Row in sections table
     * @param values :: New row values
     */
    virtual void setSectionRow(int row, SectionRow values) = 0;

    /**
     * Adds a new section selector
     * @param index :: Index of added section selector, to find it later
     * @param values :: Initial values
     */
    virtual void addSectionSelector(int index, SectionSelector values) = 0;

    /**
     * Deletes section selector at specified index
     * @param index :: Section selector index
     */
    virtual void deleteSectionSelector(int index) = 0;

    /**
     * Update section selector values
     * @param index :: Index of the selector to update
     * @param values :: New values
     */
    virtual void updateSectionSelector(int index, SectionSelector values) = 0;

    /**
     * Pops-up an error box
     * @param message :: Error message to display
     */
    virtual void displayError(const QString& message) = 0;

    /// Links help button to wiki page
    virtual void help() = 0;

  signals:
    /// Fit requested
    void fitRequested();

    /// New section addition requested
    void addSectionRequested();

    /**
     * Section removal requested
     * @param row :: Section row to remove
     */
    void removeSectionRequested(int row);

    /**
     * One of the section rows in the table was modified
     * @param row :: Modified section row
     */
    void sectionRowModified(int row);

    /**
     * One of section selectors has been modified
     * @param index :: Index of modified selector
     */
    void sectionSelectorModified(int index);
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_IALCBASELINEMODELLINGVIEW_H_ */
