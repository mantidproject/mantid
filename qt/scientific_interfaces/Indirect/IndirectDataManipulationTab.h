// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_
#include "IndirectTab.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// Number of decimal places in property browsers.
static std::size_t const DECIMAL_PLACES(6);

class MANTIDQT_INDIRECT_DLL IndirectDataManipulationTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectDataManipulationTab(QWidget *parent = nullptr);

  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr workspace);
  Mantid::API::MatrixWorkspace_sptr getInputWorkspace() const;

  int getSelectedSpectrum() const;

  void plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot);

protected:
  void updatePlotRange(std::string const &rangeName,
                       MantidQt::MantidWidgets::PreviewPlot *previewPlot,
                       std::string const &startRangePropName = "",
                       std::string const &endRangePropName = "");

  DoubleEditorFactory *m_dblEdFac;
  QtCheckBoxFactory *m_blnEdFac;

private:
  void setup() override = 0;
  void run() override = 0;
  bool validate() override = 0;
  virtual void loadSettings(QSettings const &settings) = 0;

  Mantid::API::MatrixWorkspace_sptr m_inputWorkspace;
  int m_selectedSpectrum;
  // IndirectDataManipulation *m_parent;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_ */
