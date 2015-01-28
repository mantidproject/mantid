#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_

#include "IndirectDataReductionTab.h"
#include "ui_IndirectConvertToEnergy.h"
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Background.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectConvertToEnergy

    @author Dan Nixon
    @date 23/07/2014

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport IndirectConvertToEnergy : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    IndirectConvertToEnergy(IndirectDataReduction * idrUI, QWidget * parent = 0);
    virtual ~IndirectConvertToEnergy();

    virtual void setup();
    virtual void run();

  public slots:
    virtual bool validate();

  private slots:
    void algorithmComplete(bool error);
    void setInstrumentDefault(); ///< Sets default parameters for current instrument
    void mappingOptionSelected(const QString& groupType); ///< change ui to display appropriate options
    void plotRaw(); ///< plot raw data from instrument
    void pbRunEditing();  //< Called when a user starts to type / edit the runs to load.
    void pbRunFinding();  //< Called when the FileFinder starts finding the files.
    void pbRunFinished(); //< Called when the FileFinder has finished finding the files.
    void plotRawComplete(bool error); //< Called when the Plot Raw algorithmm chain completes

  private:
    Ui::IndirectConvertToEnergy m_uiForm;

    QString createMapFile(const QString& groupType); ///< create the mapping file with which to group results
    std::vector<std::string> getSaveFormats(); ///< get a vector of save formats

  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_
