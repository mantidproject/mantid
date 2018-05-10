#include "SANSEventSlicing.h"
#include "MantidAPI/AlgorithmManager.h"
#include "ui_SANSEventSlicing.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include <stdexcept>

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::CustomInterfaces;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("SANSEventSlicing");
} // namespace

SANSEventSlicing::SANSEventSlicing(QWidget *parent) : UserSubWindow(parent) {
  setWindowFlags(windowFlags() | Qt::Dialog); //| Qt::Popup);
}

SANSEventSlicing::~SANSEventSlicing() {}

// Connect signals and setup widgets
void SANSEventSlicing::initLayout() {
  ui.setupUi(this);
  connect(ui.applyPb, SIGNAL(clicked()), this, SLOT(doApplySlice()));
  connect(ui.run_opt, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(onChangeWorkspace(const QString &)));
}

void SANSEventSlicing::onChangeWorkspace(const QString &newWS) {
  if (newWS.isEmpty())
    return;
  try {
    ChargeAndTime exp = getFullChargeAndTime(newWS);

    ui.total_label->setText(exp.charge);
    ui.time_label->setText(exp.time);
  } catch (std::runtime_error &ex) {
    raiseWarning("On load failure", ex.what());
  }
}

void SANSEventSlicing::doApplySlice() {
  QString run_name = ui.run_opt->currentText();
  if (run_name.isEmpty()) {
    raiseWarning(
        "Wrong Input",
        "Invalid run Number.\nPlease, provide a correct run number of file!");
    return;
  }
  try {
    QString code = createSliceEventCode(run_name, ui.startDouble->text(),
                                        ui.stopDouble->text());

    ChargeAndTime info = runSliceEvent(code);

    ui.sliced_label->setText(info.charge);
  } catch (std::exception &ex) {
    raiseWarning("Failed to Slice", ex.what());
  }
}

SANSEventSlicing::ChargeAndTime
SANSEventSlicing::getFullChargeAndTime(const QString &name_ws) {

  QString code;
  QTextStream stream(&code);

  stream << "import SANSUtility as su\n"
         << "import sys\n"
         << "ws = mtd['" << name_ws << "']\n"
         << "try:\n"
         << "  charge, t_passed = su.getChargeAndTime(ws)\n"
         << "  print('%.2f, %.2f' %(charge, t_passed))\n"
         << "except :\n"
         << "  print('EXCEPTION:',sys.exc_info()[1])\n";

  QString result = runPythonCode(code).simplified();

  checkPythonOutput(result);

  return values2ChargeAndTime(result);
}

SANSEventSlicing::ChargeAndTime
SANSEventSlicing::values2ChargeAndTime(const QString &input) {
  QStringList values = input.split(" ");
  ChargeAndTime inf;
  if (values.size() < 2)
    throw std::runtime_error(
        QString("Unexpected result: %1").arg(input).toStdString());

  inf.charge = values[0];
  inf.time = values[1];
  return inf;
}

void SANSEventSlicing::checkPythonOutput(const QString &result) {
  const QString MARK("EXCEPTION:");
  if (result.contains(MARK)) {

    throw std::runtime_error(QString(result).replace(MARK, "").toStdString());
  }
}

QString SANSEventSlicing::createSliceEventCode(const QString &name_ws,
                                               const QString &start,
                                               const QString &stop) {
  if (start.isEmpty() && stop.isEmpty()) {
    throw std::invalid_argument("You must provide the limits for the slicing");
  }

  QString code;
  QTextStream stream(&code);

  stream << "import sys\n"
         << "import SANSUtility as su\n"
         << "ws = mtd['" << name_ws << "']\n"
         << "outname = str(ws)+'_T'+'" << start << "'+'_T'+'" << stop << "'\n"
         << "ws = ws.clone(OutputWorkspace=outname)\n"
         << "try:\n"
         << "  mon = mtd['" << name_ws << "_monitors']\n"
         << "  hist, times = su.slice2histogram(ws"
         << ", " << start << ", " << stop << ", mon)\n"
         << "  print('%.2f, %.2f' %(times[3], times[2]))\n"
         << "except:\n"
         << "  print('EXCEPTION:',sys.exc_info()[1])";

  return code;
}

SANSEventSlicing::ChargeAndTime
SANSEventSlicing::runSliceEvent(const QString &code) {
  QString result = runPythonCode(code).simplified();

  checkPythonOutput(result);

  return values2ChargeAndTime(result);
}

void SANSEventSlicing::raiseWarning(QString title, QString message) {
  QMessageBox::warning(this, title, message);
}

void SANSEventSlicing::showEvent(QShowEvent *ev) {
  if (ui.run_opt->count() > 0)
    onChangeWorkspace(ui.run_opt->currentText());
  UserSubWindow::showEvent(ev);
}

} // namespace CustomInterfaces
} // namespace MantidQt
