// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEWLEGACY_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEWLEGACY_H_

#include "ui_IndirectFitPreviewPlot.h"

#include "DllConfig.h"
#include "IIndirectFitPlotViewLegacy.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QIcon>
#include <QPainter>
#include <QSplitterHandle>
#endif
#include <QSplitter>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// Used for painting an Icon onto the handle of the splitter on workbench
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
class SplitterHandleLegacy : public QSplitterHandle {
public:
  SplitterHandleLegacy(QIcon icon, Qt::Orientation orientation,
                 QSplitter *parent = nullptr)
      : QSplitterHandle(orientation, parent), m_icon(icon) {}

  void paintEvent(QPaintEvent *e) override {
    QSplitterHandle::paintEvent(e);

    QPainter painter(this);
    auto const xPos = static_cast<int>(std::round(this->size().width() / 2));
    m_icon.paint(&painter, xPos, -9, 24, 24);
  }

private:
  QIcon m_icon;
};

class SplitterLegacy : public QSplitter {
public:
  SplitterLegacy(QIcon icon, QWidget *parent = nullptr)
      : QSplitter(parent), m_icon(icon) {}

  QSplitterHandle *createHandle() override {
    return new SplitterHandleLegacy(m_icon, Qt::Vertical, this);
  }

private:
  QIcon m_icon;
};
#endif

class MANTIDQT_INDIRECT_DLL IndirectFitPlotViewLegacy : public IIndirectFitPlotViewLegacy {
  Q_OBJECT

public:
  IndirectFitPlotViewLegacy(QWidget *parent = nullptr);
  virtual ~IndirectFitPlotViewLegacy() override;

  void watchADS(bool watch) override;

  std::size_t getSelectedSpectrum() const override;
  int getSelectedSpectrumIndex() const override;
  int getSelectedDataIndex() const override;
  std::size_t dataSelectionSize() const override;
  bool isPlotGuessChecked() const override;

  void hideMultipleDataSelection() override;
  void showMultipleDataSelection() override;

  void setAvailableSpectra(std::size_t minimum, std::size_t maximum) override;
  void setAvailableSpectra(
      const std::vector<std::size_t>::const_iterator &from,
      const std::vector<std::size_t>::const_iterator &to) override;

  void setMinimumSpectrum(int minimum) override;
  void setMaximumSpectrum(int maximum) override;
  void setPlotSpectrum(int spectrum) override;
  void appendToDataSelection(const std::string &dataName) override;
  void setNameInDataSelection(const std::string &dataName,
                              std::size_t index) override;
  void clearDataSelection() override;

  void plotInTopPreview(const QString &name,
                        Mantid::API::MatrixWorkspace_sptr workspace,
                        std::size_t spectrum, Qt::GlobalColor colour) override;
  void plotInBottomPreview(const QString &name,
                           Mantid::API::MatrixWorkspace_sptr workspace,
                           std::size_t spectrum,
                           Qt::GlobalColor colour) override;

  void removeFromTopPreview(const QString &name) override;
  void removeFromBottomPreview(const QString &name) override;

  void enablePlotGuess(bool enable) override;
  void enableSpectrumSelection(bool enable) override;
  void enableFitRangeSelection(bool enable) override;

  void setFitSingleSpectrumText(QString const &text) override;
  void setFitSingleSpectrumEnabled(bool enable) override;

  void setBackgroundLevel(double value) override;

  void setFitRange(double minimum, double maximum) override;
  void setFitRangeMinimum(double minimum) override;
  void setFitRangeMaximum(double maximum) override;

  void setBackgroundRangeVisible(bool visible) override;
  void setHWHMRangeVisible(bool visible) override;

  void displayMessage(const std::string &message) const override;

public slots:
  void clearTopPreview() override;
  void clearBottomPreview() override;
  void clearPreviews() override;
  void setHWHMRange(double minimum, double maximum) override;
  void setHWHMMaximum(double minimum) override;
  void setHWHMMinimum(double maximum) override;

private slots:
  void setBackgroundBounds();

  void emitDelayedPlotSpectrumChanged();
  void emitPlotSpectrumChanged();
  void emitPlotSpectrumChanged(const QString &spectrum);
  void emitSelectedFitDataChanged(int /*index*/);
  void emitPlotGuessChanged(int /*doPlotGuess*/);

private:
  void createSplitterWithPlots();
  void createSplitter();
  MantidWidgets::PreviewPlot *createTopPlot();
  MantidWidgets::PreviewPlot *createBottomPlot();
  MantidWidgets::PreviewPlot *
  createPlot(MantidQt::MantidWidgets::PreviewPlot *plot,
             QSize const &minimumSize, unsigned char horizontalStretch,
             unsigned char verticalStretch) const;
  void setPlotSizePolicy(MantidQt::MantidWidgets::PreviewPlot *plot,
                         unsigned char horizontalStretch,
                         unsigned char verticalStretch) const;

  std::string getSpectrumText() const;

  void addFitRangeSelector();
  void addBackgroundRangeSelector();
  void addHWHMRangeSelector();

  std::unique_ptr<Ui::IndirectFitPreviewPlot> m_plotForm;
  std::unique_ptr<MantidQt::MantidWidgets::PreviewPlot> m_topPlot;
  std::unique_ptr<MantidQt::MantidWidgets::PreviewPlot> m_bottomPlot;
  std::unique_ptr<QSplitter> m_splitter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif