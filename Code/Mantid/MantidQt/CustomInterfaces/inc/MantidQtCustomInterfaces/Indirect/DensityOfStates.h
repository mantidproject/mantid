#ifndef MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
#define MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_

#include "ui_DensityOfStates.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSimulationTab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  class DLLExport DensityOfStates : public IndirectSimulationTab
  {
    Q_OBJECT

    public:
      DensityOfStates(QWidget * parent = 0);

      QString help() { return "DensityOfStates"; };

      void setup();
      bool validate();
      void run();

      /// Load default settings into the interface
      void loadSettings(const QSettings& settings);

      private slots:
        void dosAlgoComplete(bool error);
      void handleFileChange();
      void ionLoadComplete(bool error);

    private:
      /// The ui form
      Ui::DensityOfStates m_uiForm;
      /// Name of output workspace
      QString m_outputWsName;

  };
} // namespace CustomInterfaces
} // namespace MantidQt

#endif //MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
