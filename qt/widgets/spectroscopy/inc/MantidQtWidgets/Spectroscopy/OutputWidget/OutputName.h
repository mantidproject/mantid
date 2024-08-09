// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "ui_OutputName.h"
#include <QRegExpValidator>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
class MANTID_SPECTROSCOPY_DLL IOutputName {
public:
  ~IOutputName() = default;

  virtual int findInsertIndexLabel(QString const &basename) = 0;
  virtual std::string getCurrentLabel() const = 0;
  virtual std::string generateOutputLabel() = 0;
  virtual void generateLabelWarning() const = 0;

  virtual void setWsSuffixes(QStringList const &suffixes) = 0;
  virtual void setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix = "") = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputName final : public QWidget, public IOutputName {
  Q_OBJECT
public:
  OutputName(QWidget *parent = nullptr);
  ~OutputName() override = default;

  int findInsertIndexLabel(QString const &outputBasename) override;
  std::string getCurrentLabel() const override;
  std::string generateOutputLabel() override;
  void generateLabelWarning() const override;

  void setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix = "") override;
  void setWsSuffixes(QStringList const &suffixes) override;

private slots:
  void updateOutputLabel();

private:
  Ui::OutputName m_uiForm;
  QStringList m_suffixes;
  QString m_currBasename;
  QString m_currOutputSuffix;
};

} // namespace CustomInterfaces
} // namespace MantidQt
