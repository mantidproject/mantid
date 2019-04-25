// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/QPeaksTableModel.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include <QString>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace boost;

using Mantid::Geometry::IPeak;

namespace MantidQt {
namespace SliceViewer {

/** Convert a V3D to a comma seperated QString
 *
 * @param v3d :: the vector to convert to a QString
 * @returns :: a string representing the V3D
 */
static QString v3dAsString(const Mantid::Kernel::V3D &v3d) {
  const QString COMMA(",");
  return QString::number(v3d.X(), 'f', 4) + COMMA +
         QString::number(v3d.Y(), 'f', 4) + COMMA +
         QString::number(v3d.Z(), 'f', 4);
}

/** Get the precision to display HKL values as
 *
 * @returns :: number of decimals to round displayed HKL values to.
 */
static int getHKLPrecision() {
  auto hklPrecConfigVal =
      Mantid::Kernel::ConfigService::Instance().getValue<int>(
          "PeakColumn.hklPrec");
  int hklPrec = hklPrecConfigVal.get_value_or(2);
  return hklPrec;
}

const QString QPeaksTableModel::RUNNUMBER = "Run";
const QString QPeaksTableModel::DETID = "DetID";
const QString QPeaksTableModel::H = "h";
const QString QPeaksTableModel::K = "k";
const QString QPeaksTableModel::L = "l";
const QString QPeaksTableModel::WAVELENGTH("Wavelength");
const QString QPeaksTableModel::ENERGY_TRANSFER("delta E");
const QString QPeaksTableModel::INITIAL_ENERGY("E_i");
const QString QPeaksTableModel::FINAL_ENERGY("E_f");
const QString QPeaksTableModel::TOF("TOF");
const QString QPeaksTableModel::DSPACING = "DSpacing";
const QString QPeaksTableModel::INT = "Int";
const QString QPeaksTableModel::SIGMINT = "SigInt";
const QString QPeaksTableModel::INT_SIGINT("Int/SigInt");
const QString QPeaksTableModel::BINCOUNT("BinCount");
const QString QPeaksTableModel::BANKNAME("BankName");
const QString QPeaksTableModel::ROW("Row");
const QString QPeaksTableModel::COL("Col");
const QString QPeaksTableModel::QLAB = "QLab";
const QString QPeaksTableModel::QSAMPLE = "QSample";
const QString QPeaksTableModel::PEAKNUM = "PeakNumber";

const int QPeaksTableModel::COL_RUNNUMBER(0);
const int QPeaksTableModel::COL_DETID(1);
const int QPeaksTableModel::COL_H(2);
const int QPeaksTableModel::COL_K(3);
const int QPeaksTableModel::COL_L(4);
const int QPeaksTableModel::COL_WAVELENGTH(5);
const int QPeaksTableModel::COL_INITIAL_ENERGY(6);
const int QPeaksTableModel::COL_FINAL_ENERGY(7);
const int QPeaksTableModel::COL_ENERGY_TRANSFER(8);
const int QPeaksTableModel::COL_TOF(9);
const int QPeaksTableModel::COL_DSPACING(10);
const int QPeaksTableModel::COL_INT(11);
const int QPeaksTableModel::COL_SIGMINT(12);
const int QPeaksTableModel::COL_INT_SIGINT(13);
const int QPeaksTableModel::COL_BINCOUNT(14);
const int QPeaksTableModel::COL_BANKNAME(15);
const int QPeaksTableModel::COL_ROW(16);
const int QPeaksTableModel::COL_COL(17);
const int QPeaksTableModel::COL_QLAB(18);
const int QPeaksTableModel::COL_QSAMPLE(19);
const int QPeaksTableModel::COL_PEAKNUM(20);

/**
 * @param column The column to get the number of characters
 * estimated for.
 *
 * @return The number of characters estimated for the column. If
 * it cannot be determined this returns 10.
 */
int QPeaksTableModel::numCharacters(const int column) const {
  if (column == COL_RUNNUMBER)
    return 5;
  else if (column == COL_DETID)
    return 7;
  else if (column == COL_H)
    return 3 + m_hklPrec;
  else if (column == COL_K)
    return 3 + m_hklPrec;
  else if (column == COL_L)
    return 3 + m_hklPrec;
  else if (column == COL_WAVELENGTH)
    return 6;
  else if (column == COL_ENERGY_TRANSFER)
    return 6;
  else if (column == COL_INITIAL_ENERGY)
    return 6;
  else if (column == COL_FINAL_ENERGY)
    return 6;
  else if (column == COL_TOF)
    return 6;
  else if (column == COL_DSPACING)
    return 6;
  else if (column == COL_INT)
    return 5;
  else if (column == COL_SIGMINT)
    return 5;
  else if (column == COL_INT_SIGINT)
    return 5;
  else if (column == COL_BINCOUNT)
    return 6;
  else if (column == COL_BANKNAME)
    return 6;
  else if (column == COL_ROW)
    return 3;
  else if (column == COL_COL)
    return 3;
  else if (column == COL_QLAB)
    return 3 * 6;
  else if (column == COL_QSAMPLE)
    return 3 * 6;
  else if (column == COL_PEAKNUM)
    return 6;
  else
    return 3;
}

/**
Constructor
@param peaksWS : Workspace model.
*/
QPeaksTableModel::QPeaksTableModel(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS)
    : QAbstractTableModel(nullptr), m_peaksWS(peaksWS) {
  m_columnNameMap = {{0, RUNNUMBER},
                     {1, DETID},
                     {2, H},
                     {3, K},
                     {4, L},
                     {5, WAVELENGTH},
                     {6, INITIAL_ENERGY},
                     {7, FINAL_ENERGY},
                     {8, ENERGY_TRANSFER},
                     {9, TOF},
                     {10, DSPACING},
                     {11, INT},
                     {12, SIGMINT},
                     {13, INT_SIGINT},
                     {14, BINCOUNT},
                     {15, BANKNAME},
                     {16, ROW},
                     {17, COL},
                     {18, QLAB},
                     {19, QSAMPLE},
                     {20, PEAKNUM}};

  m_hklPrec = getHKLPrecision();

  // Utility function to convert a V3D type to a QString by combining the
  // elements with a comma

  // Mapping member functions of the Peak object to a column index
  m_dataLookup = {
      [](const IPeak &peak) { return QVariant(peak.getRunNumber()); },
      [](const IPeak &peak) { return QVariant(peak.getDetectorID()); },
      [](const IPeak &peak) { return QVariant(peak.getH()); },
      [](const IPeak &peak) { return QVariant(peak.getK()); },
      [](const IPeak &peak) { return QVariant(peak.getL()); },
      [](const IPeak &peak) { return QVariant(peak.getWavelength()); },
      [](const IPeak &peak) { return QVariant(peak.getInitialEnergy()); },
      [](const IPeak &peak) { return QVariant(peak.getFinalEnergy()); },
      [](const IPeak &peak) { return QVariant(peak.getEnergyTransfer()); },
      [](const IPeak &peak) { return QVariant(peak.getTOF()); },
      [](const IPeak &peak) { return QVariant(peak.getDSpacing()); },
      [](const IPeak &peak) { return QVariant(peak.getIntensity()); },
      [](const IPeak &peak) { return QVariant(peak.getSigmaIntensity()); },
      [](const IPeak &peak) { return QVariant(peak.getIntensityOverSigma()); },
      [](const IPeak &peak) { return QVariant(peak.getBinCount()); },
      [](const IPeak &peak) {
        return QVariant(QString::fromStdString(peak.getBankName()));
      },
      [](const IPeak &peak) { return QVariant(peak.getRow()); },
      [](const IPeak &peak) { return QVariant(peak.getCol()); },
      [](const IPeak &peak) { return QVariant(peak.getQLabFrame().norm()); },
      [](const IPeak &peak) { return QVariant(peak.getQSampleFrame().norm()); },
      [](const IPeak &peak) { return QVariant(int(peak.getPeakNumber())); },
  };

  // Mapping member functions of the Peak object to a column index with
  // formatting for displaying data to the user
  m_formattedValueLookup = {
      [](const IPeak &peak) { return QString::number(peak.getRunNumber()); },
      [](const IPeak &peak) { return QString::number(peak.getDetectorID()); },
      [](const IPeak &peak) {
        return QString::number(peak.getH(), 'f', getHKLPrecision());
      },
      [](const IPeak &peak) {
        return QString::number(peak.getK(), 'f', getHKLPrecision());
      },
      [](const IPeak &peak) {
        return QString::number(peak.getL(), 'f', getHKLPrecision());
      },
      [](const IPeak &peak) {
        return QString::number(peak.getWavelength(), 'f', 4);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getInitialEnergy(), 'f', 4);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getFinalEnergy(), 'f', 4);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getEnergyTransfer(), 'f', 4);
      },
      [](const IPeak &peak) { return QString::number(peak.getTOF(), 'f', 1); },
      [](const IPeak &peak) {
        return QString::number(peak.getDSpacing(), 'f', 4);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getIntensity(), 'f', 1);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getSigmaIntensity(), 'f', 1);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getIntensityOverSigma(), 'f', 2);
      },
      [](const IPeak &peak) {
        return QString::number(peak.getBinCount(), 'g', 2);
      },
      [](const IPeak &peak) {
        return QString::fromStdString(peak.getBankName());
      },
      [](const IPeak &peak) { return QString::number(peak.getRow()); },
      [](const IPeak &peak) { return QString::number(peak.getCol()); },
      [](const IPeak &peak) { return v3dAsString(peak.getQLabFrame()); },
      [](const IPeak &peak) { return v3dAsString(peak.getQSampleFrame()); },
      [](const IPeak &peak) { return QString::number(peak.getPeakNumber()); },
  };
}

void QPeaksTableModel::setPeaksWorkspace(
    boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) {
  beginResetModel();
  emit layoutAboutToBeChanged();
  m_peaksWS = peaksWS;
  emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
  emit layoutChanged();
  endResetModel();
}

/**
@return the row count.
*/
int QPeaksTableModel::rowCount(const QModelIndex & /*parent*/) const {
  return static_cast<int>(m_peaksWS->rowCount());
}

/**
@return the number of columns in the model.
*/
int QPeaksTableModel::columnCount(const QModelIndex & /*parent*/) const {
  return static_cast<int>(m_dataLookup.size());
}

/**
Find the column name at a given column index.
@param colIndex : Index to find column name for.
*/
QString QPeaksTableModel::findColumnName(const int colIndex) const {
  ColumnIndexNameMap::const_iterator foundColName =
      m_columnNameMap.find(colIndex);
  if (foundColName == m_columnNameMap.end()) {
    throw std::runtime_error("Unknown column requested");
  }
  return foundColName->second;
}

/**
Overrident data method, allows consuming view to extract data for an index and
role.
@param index : For which to extract the data
@param role : Role mode
*/
QVariant QPeaksTableModel::data(const QModelIndex &index, int role) const {

  if (role == Qt::TextAlignmentRole)
    return Qt::AlignRight;

  const auto colNumber = index.column();
  const auto rowNumber = index.row();

  const auto &peak = m_peaksWS->getPeak(rowNumber);

  if (role == Qt::DisplayRole) {
    // For displaying values to the user
    return m_formattedValueLookup[colNumber](peak);
  } else if (role == Qt::UserRole) {
    // For using values for searching
    return m_dataLookup[colNumber](peak);
  }

  return QVariant();
}

/**
Get the heading for a given section, orientation and role.
@param section : Column index
@param orientation : Heading orientation
@param role : Role mode of table.
@return HeaderData.
*/
QVariant QPeaksTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (role != Qt::DisplayRole || section < 0)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    return findColumnName(section);
  }
  return QVariant();
}

/**
Provide flags on an index by index basis
@param index: To generate a flag for.
*/
Qt::ItemFlags QPeaksTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * @return List of columns to hide by default.
 */
std::vector<int> QPeaksTableModel::defaultHideCols() {
  std::vector<int> result;

  // only show bank name for SNS instruments
  Mantid::Geometry::Instrument_const_sptr instr = m_peaksWS->getInstrument();
  std::string instrName = instr->getName();
  try {
    Mantid::Kernel::InstrumentInfo instrInfo =
        Mantid::Kernel::ConfigService::Instance().getInstrument(instrName);
    if (instrInfo.facility().name() != "SNS")
      result.push_back(COL_BANKNAME);

    // hide some columns based on the techniques
    { // shrink variable scope
      auto techniques = instrInfo.techniques();
      // required for showing final and delta energy
      const std::string IGS("TOF Indirect Geometry Spectroscopy");
      // required for showing initial and delta energy
      const std::string DGS("TOF Direct Geometry Spectroscopy");
      bool showEnergy(false);
      if (techniques.find(DGS) == techniques.end())
        result.push_back(COL_FINAL_ENERGY);
      else
        showEnergy = true;
      if (techniques.find(IGS) == techniques.end())
        result.push_back(COL_INITIAL_ENERGY);
      else
        showEnergy = true;
      if (!showEnergy)
        result.push_back(COL_ENERGY_TRANSFER);
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    // Unable to fetch instrument info, so continue without it.
  }

  return result;
}

/// Destructor
QPeaksTableModel::~QPeaksTableModel() {}
} // namespace SliceViewer
} // namespace MantidQt
