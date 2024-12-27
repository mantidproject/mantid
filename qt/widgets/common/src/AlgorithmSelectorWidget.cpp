// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmSelectorWidget.h"
#include "MantidAPI/AlgorithmManager.h"

#include "boost/algorithm/string.hpp"

#include <QApplication>
#include <QCompleter>
#include <QDrag>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt::MantidWidgets {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AlgorithmSelectorWidget::AlgorithmSelectorWidget(QWidget *parent)
    : QWidget(parent), m_tree(nullptr), m_findAlg(nullptr), m_execButton(nullptr),
      m_updateObserver(*this, &AlgorithmSelectorWidget::handleAlgorithmFactoryUpdate), m_updateInProgress(false) {
  auto *buttonLayout = new QHBoxLayout();

  m_tree = new AlgorithmTreeWidget(this);
  m_tree->setHeaderLabel("Algorithms");
  connect(m_tree, SIGNAL(itemSelectionChanged()), this, SLOT(treeSelectionChanged()));
  connect(m_tree, SIGNAL(executeAlgorithm(const QString &, int)), this, SLOT(executeSelected()));

  m_findAlg = new FindAlgComboBox;
  m_findAlg->setEditable(true);
  m_findAlg->completer()->setCompletionMode(QCompleter::PopupCompletion);

  // Make the algorithm drop down use all the sapce it can horizontally
  QSizePolicy expandHoriz;
  expandHoriz.setHorizontalPolicy(QSizePolicy::Expanding);
  m_findAlg->setSizePolicy(expandHoriz);

  connect(m_findAlg, SIGNAL(enterPressed()), this, SLOT(executeSelected()));
  connect(m_findAlg, SIGNAL(editTextChanged(const QString &)), this, SLOT(findAlgTextChanged(const QString &)));

  m_execButton = new QPushButton("Execute");
  connect(m_execButton, SIGNAL(clicked()), this, SLOT(executeSelected()));
  buttonLayout->addWidget(m_execButton);

  buttonLayout->addWidget(m_findAlg);

  // Layout the tree and combo box
  QVBoxLayout *layout = new QVBoxLayout(this);
  // this->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);

  // The poco notification will be dispatched from the callers thread but we
  // need to
  // make sure the updates to the widgets happen on the GUI thread. Dispatching
  // through a Qt signal will make sure it is in the correct thread.
  AlgorithmFactory::Instance().notificationCenter.addObserver(m_updateObserver);
  connect(this, SIGNAL(algorithmFactoryUpdateReceived()), this, SLOT(update()));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AlgorithmSelectorWidget::~AlgorithmSelectorWidget() {
  AlgorithmFactory::Instance().notificationCenter.removeObserver(m_updateObserver);
}

/** Is the execute button visible */
bool AlgorithmSelectorWidget::showExecuteButton() const { return m_execButton->isVisible(); }

/** Show/hide the execute button */
void AlgorithmSelectorWidget::showExecuteButton(const bool val) { m_execButton->setVisible(val); }

//---------------------------------------------------------------------------
/** Update the lists of algorithms */
void AlgorithmSelectorWidget::update() {
  m_updateInProgress = true;
  m_findAlg->update();
  m_tree->update();
  m_updateInProgress = false;
}

//---------------------------------------------------------------------------
/** Slot called to execute whatever is the selected algorithm
 **/
void AlgorithmSelectorWidget::executeSelected() {
  auto alg = this->getSelectedAlgorithm();
  if (!alg.name.isEmpty()) {
    emit executeAlgorithm(alg.name, alg.version);
  }
}

//---------------------------------------------------------------------------
/** Show the selection in the tree when it changes in the combo */
void AlgorithmSelectorWidget::findAlgTextChanged(const QString &text) {
  int i = m_findAlg->findText(text, Qt::MatchFixedString);
  if (i >= 0)
    m_findAlg->setCurrentIndex(i);
  // De-select from the tree
  m_tree->blockSignals(true);
  m_tree->setCurrentIndex(QModelIndex());
  m_tree->blockSignals(false);

  // Emit the signal
  auto alg = this->getSelectedAlgorithm();
  emit algorithmSelectionChanged(alg.name, alg.version);
}

//---------------------------------------------------------------------------
/** Show the selection in the combo when it changes in the tree */
void AlgorithmSelectorWidget::treeSelectionChanged() {
  auto alg = this->getSelectedAlgorithm();
  // Select in the combo box
  m_findAlg->blockSignals(true);
  m_findAlg->setCurrentIndex(m_findAlg->findText(alg.name, Qt::MatchFixedString));
  m_findAlg->blockSignals(false);
  // Emit the signal
  emit algorithmSelectionChanged(alg.name, alg.version);
}

//---------------------------------------------------------------------------
/** Return the selected algorithm.
 * The tree has priority. If nothing is selected in the tree,
 * return the ComboBox selection */
SelectedAlgorithm AlgorithmSelectorWidget::getSelectedAlgorithm() {
  SelectedAlgorithm alg = m_tree->getSelectedAlgorithm();
  if (alg.name.isEmpty())
    alg = m_findAlg->getSelectedAlgorithm();
  return alg;
}

//---------------------------------------------------------------------------
/** Set which algorithm is currently selected. Does not fire any signals.
 * Updates the combobox, deselects in the tree.
 *
 * @param algName :: name of the algorithm
 */
void AlgorithmSelectorWidget::setSelectedAlgorithm(QString &algName) {
  m_findAlg->blockSignals(true);
  m_findAlg->setCurrentIndex(m_findAlg->findText(algName, Qt::MatchFixedString));
  m_findAlg->blockSignals(false);
  // De-select from the tree
  m_tree->blockSignals(true);
  m_tree->setCurrentIndex(QModelIndex());
  m_tree->blockSignals(false);
}

/**
 * The algorithm factory has been updated, refresh the widget
 *
 */
void AlgorithmSelectorWidget::handleAlgorithmFactoryUpdate(
    Mantid::API::AlgorithmFactoryUpdateNotification_ptr /*unused*/) {
  emit algorithmFactoryUpdateReceived();
}

//============================================================================
//============================================================================
//============================================================================
// Use an anonymous namespace to keep these at file scope
namespace {

bool AlgorithmDescriptorLess(const AlgorithmDescriptor &d1, const AlgorithmDescriptor &d2) {
  if (d1.category < d2.category)
    return true;
  else if (d1.category == d2.category && d1.name < d2.name)
    return true;
  else if (d1.category == d2.category && d1.name == d2.name && d1.version > d2.version)
    return true;

  return false;
}

bool AlgorithmDescriptorNameLess(const AlgorithmDescriptor &d1, const AlgorithmDescriptor &d2) {
  return d1.name < d2.name;
}
} // namespace

//============================================================================
//======================= AlgorithmTreeWidget ================================
//============================================================================
/** Return the selected algorithm in the tree
 * @returns :: algorithm selected by user
 */
SelectedAlgorithm AlgorithmTreeWidget::getSelectedAlgorithm() {
  SelectedAlgorithm alg("", 0);

  auto isCategoryName = [](const QTreeWidgetItem *item) {
    return (item->childCount() != 0 && !item->text(0).contains(" v."));
  };

  QList<QTreeWidgetItem *> items = this->selectedItems();
  if (items.size() > 0 && !isCategoryName(items[0])) {
    QString str = items[0]->text(0);
    QStringList lst = str.split(" v.");
    alg.name = lst[0];
    alg.version = lst[1].toInt();
  }
  return alg;
}

//---------------------------------------------------------------------------
/** SLOT called when clicking the mouse around the tree */
void AlgorithmTreeWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    if (!itemAt(e->pos()))
      selectionModel()->clear();
    m_dragStartPosition = e->pos();
  }

  QTreeWidget::mousePressEvent(e);
}

//---------------------------------------------------------------------------
/** SLOT called when dragging the mouse around the tree */
void AlgorithmTreeWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!(e->buttons() & Qt::LeftButton))
    return;
  if ((e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    return;

  // Start dragging
  QDrag *drag = new QDrag(this);
  auto *mimeData = new QMimeData;

  mimeData->setText("Algorithm");
  drag->setMimeData(mimeData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void)dropAction;
}

//---------------------------------------------------------------------------
/** SLOT called when double-clicking on an entry in the tree */
void AlgorithmTreeWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  auto alg = this->getSelectedAlgorithm();
  if (!alg.name.isEmpty()) {
    // Emit the signal that we are executing
    emit executeAlgorithm(alg.name, alg.version);
    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

//---------------------------------------------------------------------------
/** Update the list of algos in the tree */
void AlgorithmTreeWidget::update() {
  this->clear();

  using AlgNamesType = std::vector<AlgorithmDescriptor>;
  AlgNamesType names = AlgorithmFactory::Instance().getDescriptors(false, false);

  // sort by category/name/version to fill QTreeWidget
  sort(names.begin(), names.end(), AlgorithmDescriptorLess);

  QMap<QString, QTreeWidgetItem *> categories; // keeps track of categories added to the tree
  QMap<QString, QTreeWidgetItem *> algorithms; // keeps track of algorithms
                                               // added to the tree (needed in
                                               // case there are different
                                               // versions of an algorithm)

  for (AlgNamesType::const_iterator i = names.begin(); i != names.end(); ++i) {
    QString algName = QString::fromStdString(i->name);
    QString catName = QString::fromStdString(i->category);
    QStringList subCats = catName.split('\\');
    if (!categories.contains(catName)) {
      if (subCats.size() == 1) {
        QTreeWidgetItem *catItem = new QTreeWidgetItem(QStringList(catName));
        categories.insert(catName, catItem);
        this->addTopLevelItem(catItem);
      } else {
        QString cn = subCats[0];
        QTreeWidgetItem *catItem = nullptr;
        int n = subCats.size();
        for (int j = 0; j < n; j++) {
          if (categories.contains(cn)) {
            catItem = categories[cn];
          } else {
            QTreeWidgetItem *newCatItem = new QTreeWidgetItem(QStringList(subCats[j]));
            categories.insert(cn, newCatItem);
            if (!catItem) {
              this->addTopLevelItem(newCatItem);
            } else {
              catItem->addChild(newCatItem);
            }
            catItem = newCatItem;
          }
          if (j != n - 1)
            cn += "\\" + subCats[j + 1];
        }
      }
    }

    QTreeWidgetItem *algItem = new QTreeWidgetItem(QStringList(algName + " v." + QString::number(i->version)));
    QString cat_algName = catName + algName;
    if (!algorithms.contains(cat_algName)) {
      algorithms.insert(cat_algName, algItem);
      categories[catName]->addChild(algItem);
    } else
      algorithms[cat_algName]->addChild(algItem);
  }
}

//============================================================================
//============================== FindAlgComboBox =============================
//============================================================================
/** Called when the combo box for finding algorithms has a key press
 * event  */
void FindAlgComboBox::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Return) {
    emit enterPressed();
    return;
  }
  QComboBox::keyPressEvent(e);
}

//---------------------------------------------------------------------------
/** Update the list of algos in the combo box */
void FindAlgComboBox::update() {
  // include hidden categories in the combo list box
  AlgNamesType names = AlgorithmFactory::Instance().getDescriptors(true, false);
  addAliases(names);

  // sort by algorithm names only to fill this combobox
  sort(names.begin(), names.end(), AlgorithmDescriptorNameLess);

  this->clear();
  std::string prevName = "";
  for (AlgNamesType::const_iterator i = names.begin(); i != names.end(); ++i) {
    if (i->name != prevName)
      this->addItem(QString::fromStdString(i->name));
    prevName = i->name;
  }
  this->setCurrentIndex(-1);
}

/** Adds alias entries to the list of algorithms */
void FindAlgComboBox::addAliases(AlgNamesType &algNamesList) {
  AlgNamesType aliasList;
  for (AlgNamesType::const_iterator i = algNamesList.begin(); i != algNamesList.end(); ++i) {
    // the alias is not empty and is not just different by case from the name
    if ((!i->alias.empty()) && (!boost::iequals(i->alias, i->name))) {
      AlgorithmDescriptor newAlias(*i);
      newAlias.name = i->alias + " [" + i->name + "]";
      aliasList.emplace_back(newAlias);
    }
  }
  // add them to the list - unsorted
  algNamesList.reserve(algNamesList.size() + aliasList.size());
  algNamesList.insert(algNamesList.end(), aliasList.begin(), aliasList.end());
}

/** if a string is for an alias convert it to the algorithm name */
QString FindAlgComboBox::stripAlias(const QString &text) const {
  QString retVal = text;
  int foundOpen = text.indexOf("[");
  if (foundOpen != -1) {
    int foundClose = text.lastIndexOf("]");
    if (foundClose != -1)
      retVal = text.mid(foundOpen + 1, foundClose - foundOpen - 1);
  }
  return retVal;
}

//---------------------------------------------------------------------------
/** Return the selected algorithm */
SelectedAlgorithm FindAlgComboBox::getSelectedAlgorithm() {
  // typed selection
  QString typedText = this->currentText().trimmed(); // text as typed in the combobox
  if (!typedText.isEmpty())                          // if the text is not empty
  {
    // find the closest matching entry
    int matchedIndex = this->findText(typedText, Qt::MatchStartsWith);
    if (matchedIndex > -1) {
      typedText = this->itemText(matchedIndex); // text in the combobox at the matched index
      typedText = stripAlias(typedText);
    }
  }
  return SelectedAlgorithm(typedText, -1);
}

} // namespace MantidQt::MantidWidgets
