#ifndef MANTID_CUSTOMINTERFACES_C2ETAB_H_
#define MANTID_CUSTOMINTERFACES_C2ETAB_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/ConvertToEnergy.h"

namespace MantidQt
{
namespace CustomInterfaces
{


  /** C2ETab : TODO: DESCRIPTION
    
    @author Samuel Jackson
    @date 13/08/2013

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
  class DLLExport C2ETab : public QWidget
  {
    Q_OBJECT

  public:
    C2ETab(Ui::ConvertToEnergy& uiForm, QWidget * parent = 0);
    virtual ~C2ETab();
    void runTab();
    void setupTab();
    void validateTab();


  signals:
   void runAsPythonScript(const QString & code, bool no_output);

  private:
    /// Overidden by child class.
    virtual void setup() = 0;
    /// Overidden by child class.
    virtual void run() = 0;
    /// Overidden by child class.
    virtual bool validate() = 0;

  protected:
    Ui::ConvertToEnergy m_uiForm;
  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_C2ETAB_H_ */
