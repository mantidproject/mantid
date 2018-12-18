// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGETENCODER_H_
#define INSTRUMENTWIDGETENCODER_H_
namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetEncoder {
public:
  QMap<QString, QVariant> encode(const InstrumentWidget &obj);
  
private:
  // Save surface details
  QMap<QString, QVariant> encodeSurface(const InstrumentWidget &obj);
  
  // Encode Actor
  QMap<QString, QVariant> encodeActor(const InstrumentWidget &obj);
  
  // Encode all tabs
  QMap<QString, QVariant> encodeTabs(const InstrumentWidget &obj);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INSTRUMENTWIDGETENCODER_H_*/