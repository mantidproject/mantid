//----------------------------------
// Includes
//----------------------------------
#include "MantidWSIndexDialog.h"
#include "MantidUI.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorTypes.h"

#include <stdlib.h>
#include <QRegExp>
#include <QtAlgorithms> 
#include <boost/lexical_cast.hpp>
#include <exception>
#include <QPalette>

//----------------------------------
// MantidWSIndexDialog public methods
//----------------------------------
/**
 * Construct an object of this type
 * @param mui :: The MantidUI area
 * @param flags :: Window flags that are passed the the QDialog constructor
 * @param wsNames :: the names of the workspaces to be plotted
 * @param showWaterfallOption :: If true the waterfall checkbox is created
 */
MantidWSIndexDialog::MantidWSIndexDialog(MantidUI* mui, Qt::WFlags flags, QList<QString> wsNames, const bool showWaterfallOption) 
  : QDialog(mui->appWindow(), flags), 
  m_mantidUI(mui),
  m_spectra(false),
  m_waterfall(showWaterfallOption),
  m_wsNames(wsNames),
  m_wsIndexIntervals(),
  m_spectraIdIntervals(),
  m_wsIndexChoice(), 
  m_spectraIdChoice()
{
  checkForSpectraAxes();

  // Generate the intervals allowed to be plotted by the user.
  generateWsIndexIntervals();
  if(m_spectra)
  {
    generateSpectraIdIntervals();
  }
  // Set up UI.
  init();
}

MantidWSIndexDialog::UserInput MantidWSIndexDialog::getSelections() const
{
  UserInput options;
  options.plots = getPlots();
  options.waterfall = waterfallPlotRequested();
  return options;
}

QMultiMap<QString,std::set<int> > MantidWSIndexDialog::getPlots() const
{
  // Map of workspace names to set of indices to be plotted.
  QMultiMap<QString,std::set<int> > plots;

  // If the user typed in the wsField ...
  if(m_wsIndexChoice.getList().size() > 0)
  {
    
    for(int i = 0; i < m_wsNames.size(); i++)
    {
      std::set<int> intSet = m_wsIndexChoice.getIntSet();
      plots.insert(m_wsNames[i],intSet);
    }
  }
  // Else if the user typed in the spectraField ...
  else if(m_spectraIdChoice.getList().size() > 0)
  {
    for(int i = 0; i < m_wsNames.size(); i++)
    {
      // Convert the spectra choices of the user into workspace indices for us to use.
      Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(m_wsNames[i].toStdString()));
      if ( NULL == ws ) continue;

      const Mantid::spec2index_map spec2index = ws->getSpectrumToWorkspaceIndexMap();

      std::set<int> origSet = m_spectraIdChoice.getIntSet();
      std::set<int>::iterator it = origSet.begin();
      std::set<int> convertedSet;

      for( ; it != origSet.end(); ++it)
      {
        int origInt = (*it);
        int convertedInt = static_cast<int>(spec2index.find(origInt)->second);
        convertedSet.insert(convertedInt);
      }

      plots.insert(m_wsNames[i],convertedSet);
    }
  }

  return plots;
}

bool MantidWSIndexDialog::waterfallPlotRequested() const
{
  return m_waterfallOpt->isChecked();
}

//----------------------------------
// MantidWSIndexDialog private slots
//----------------------------------
void MantidWSIndexDialog::plot()
{    
  int npos = 0;
  QString wsText = m_wsField->lineEdit()->text();
  QString spectraTest = m_spectraField->lineEdit()->text();
  QValidator::State wsState = m_wsField->lineEdit()->validator()->validate(wsText, npos);
  QValidator::State spectraState = m_spectraField->lineEdit()->validator()->validate(spectraTest, npos);
  
  // If the user typed in the wsField ...
  if(wsState == QValidator::Acceptable)
  {
    m_wsIndexChoice.addIntervals(m_wsField->lineEdit()->text());
    accept();
  }
  // Else if the user typed in the spectraField ...
  else if(spectraState == QValidator::Acceptable)
  {
    m_spectraIdChoice.addIntervals(m_spectraField->lineEdit()->text());
    accept();
  }else{
    QString error_message("Invalid input. It is not in the range available"); 
    if (!wsText.isEmpty())
      m_wsField->setError(error_message); 
    if (!spectraTest.isEmpty())
      m_spectraField->setError(error_message); 
  }
}

void MantidWSIndexDialog::plotAll()
{
  m_wsIndexChoice = m_wsIndexIntervals;
  accept();
}

void MantidWSIndexDialog::editedWsField()
{
  if(usingSpectraIDs()) {
    m_spectraField->lineEdit()->clear();
    m_spectraField->setError(""); 
  }
}

void MantidWSIndexDialog::editedSpectraField()
{
  m_wsField->lineEdit()->clear();
  m_wsField->setError(""); 
}

//----------------------------------
// MantidWSIndexDialog private methods
//----------------------------------
void MantidWSIndexDialog::init()
{
  m_outer = new QVBoxLayout;

  setWindowTitle(tr("MantidPlot"));
  initSpectraBox();
  initWorkspaceBox();
  initOptionsBoxes();
  initButtons();
  setLayout(m_outer);
}

void MantidWSIndexDialog::initWorkspaceBox()
{
  m_wsBox = new QVBoxLayout;
  m_wsMessage = new QLabel(tr("Enter Workspace Indices: " + m_wsIndexIntervals.toQString()));
  m_wsField = new QLineEditWithErrorMark();

  m_wsField->lineEdit()->setValidator(new IntervalListValidator(this, m_wsIndexIntervals));
  m_wsBox->add(m_wsMessage);
  m_wsBox->add(m_wsField);
  m_outer->addItem(m_wsBox);

  connect(m_wsField->lineEdit(), SIGNAL(textEdited(const QString &)), this, SLOT(editedWsField()));
}

void MantidWSIndexDialog::initSpectraBox()
{
  m_spectraBox = new QVBoxLayout;
  m_spectraMessage = new QLabel(tr("Enter Spectra IDs: " + m_spectraIdIntervals.toQString()));
  m_spectraField = new QLineEditWithErrorMark();
  m_orMessage = new QLabel(tr("<br>Or"));

  m_spectraField->lineEdit()->setValidator(new IntervalListValidator(this, m_spectraIdIntervals));
  m_spectraBox->add(m_spectraMessage);
  m_spectraBox->add(m_spectraField);
  m_spectraBox->add(m_orMessage);
  
  if( usingSpectraIDs() )
    m_outer->addItem(m_spectraBox);

  connect(m_spectraField->lineEdit(), SIGNAL(textEdited(const QString &)), this, SLOT(editedSpectraField()));
}

void MantidWSIndexDialog::initOptionsBoxes()
{
  m_optionsBox = new QHBoxLayout;
  m_waterfallOpt = new QCheckBox("Waterfall Plot");
  if(m_waterfall)
    m_optionsBox->add(m_waterfallOpt);
  else
    m_waterfallOpt->setChecked(true);

  m_outer->addItem(m_optionsBox);
}

void MantidWSIndexDialog::initButtons()
{
  m_buttonBox = new QHBoxLayout;
  
  m_okButton = new QPushButton("OK");
  m_cancelButton = new QPushButton("Cancel");
  m_plotAllButton = new QPushButton("Plot All");

  m_buttonBox->addWidget(m_okButton);
  m_buttonBox->addWidget(m_cancelButton);
  m_buttonBox->addWidget(m_plotAllButton);

  m_outer->addItem(m_buttonBox);

  connect(m_okButton, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(m_plotAllButton, SIGNAL(clicked()), this, SLOT(plotAll()));
}

void MantidWSIndexDialog::checkForSpectraAxes()
{
  // Check to see if *all* workspaces have a spectrum axis.
  // If even one does not have a spectra axis, then we wont
  // ask the user to enter spectra IDs - only workspace indices.
  QList<QString>::const_iterator it = m_wsNames.constBegin();
  m_spectra = true;

  for ( ; it != m_wsNames.constEnd(); ++it )
  {
    Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if ( NULL == ws ) continue;
    bool hasSpectra = false;
    for(int i = 0; i < ws->axes(); i++)
    {
      if(ws->getAxis(i)->isSpectra()) 
        hasSpectra = true;
    }
    if(hasSpectra == false)
    {
      m_spectra = false;
      break;
    }
  }
}

void MantidWSIndexDialog::generateWsIndexIntervals()
{
  // Get the available interval for each of the workspaces, and then
  // present the user with interval which is the INTERSECTION of each of
  // those intervals.
  QList<QString>::const_iterator it = m_wsNames.constBegin();
  
  // Cycle through the workspaces ...
  for ( ; it != m_wsNames.constEnd(); ++it )
  {
    Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if ( NULL == ws ) continue;
    
    const int endWs = static_cast<int>(ws->getNumberHistograms() - 1);//= static_cast<int> (end->first);

    Interval interval(0,endWs);
    // If no interval has been added yet, just add it ...
    if(it == m_wsNames.constBegin())
      m_wsIndexIntervals.addInterval(interval);
    // ... else set the list as the intersection of what's already there
    // and what has just been added.
    else
      m_wsIndexIntervals.setIntervalList(IntervalList::intersect(m_wsIndexIntervals,interval));
  }
}

void MantidWSIndexDialog::generateSpectraIdIntervals()
{
  bool firstWs = true;
  foreach( const QString wsName, m_wsNames )
  {
    Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    if( !ws ) continue; // Belt and braces.

    const Mantid::spec2index_map spec2index = ws->getSpectrumToWorkspaceIndexMap();

    IntervalList spectraIntervalList;
    for( auto pair = spec2index.begin(); pair != spec2index.end(); ++pair )
    {
      spectraIntervalList.addInterval(static_cast<int>(pair->first));
    }

    if( firstWs )
    {
      m_spectraIdIntervals = spectraIntervalList;
      firstWs = false;
    }
    else
    {
      m_spectraIdIntervals.setIntervalList(IntervalList::intersect(m_spectraIdIntervals, spectraIntervalList));
    }
  }
}

bool MantidWSIndexDialog::usingSpectraIDs() const
{
  return m_spectra && m_spectraIdIntervals.getList().size() > 0;
}

//----------------------------------
// Interval public methods
//----------------------------------
Interval::Interval(int single)
{
  init(single, single);
}

Interval::Interval(int start, int end)
{
  init(start, end);
}

Interval::Interval(QString intervalString)
{
  // Check to see if string is of the correct format, and then parse.
  // An interval can either be "n" or "n-m" where n and m are integers
  const QString patternSingle("^\\d+$");     // E.g. "2" or "712"
  const QString patternRange("^\\d+-\\d+$"); // E.g. "2-4" or "214-200"
  const QRegExp regExpSingle(patternSingle);
  const QRegExp regExpRange(patternRange);
  
  if(regExpSingle.exactMatch(intervalString))
  {
    int single = intervalString.toInt();
    init(single, single);
  }
  else if(regExpRange.exactMatch(intervalString))
  {
    QStringList range = intervalString.split("-");
    int start = range[0].toInt();
    int end = range[1].toInt();
    init(start, end);
  }
  else
  {
    throw std::exception();
  }
}

Interval::Interval(const Interval& copy)
{
  init(copy.m_start, copy.m_end);
}

bool Interval::merge(const Interval& other) 
{
  // If cant merge, return false
  if(!canMerge(other))
    return false;

  // Else, merge - e.g "2" into "3-5" to create "2-5":

  if(other.start() < m_start)
    m_start = other.start();

  if(other.end() > m_end)
    m_end = other.end();

  return true;
}

bool Interval::canMerge(const Interval& other) const
{
  if(other.start() > m_end + 1 || other.end() + 1 < m_start)
    return false;
  else
    return true;
}

int Interval::start() const
{
  return m_start;
}

int Interval::end() const
{
  return m_end;
}

// Note that the length of an interval with only one number is 1.
// Ergo, "length" is defined as (1 + (end - start))
int Interval::length() const
{
  return 1 + m_end - m_start;
}

std::set<int> Interval::getIntSet() const
{
  std::set<int> intSet;

  for(int i = m_start; i <= m_end; i++)
  {
    intSet.insert(i);
  }

  return intSet;
}

bool Interval::contains(const Interval& other) const
{
  if(other.m_start >= m_start && other.m_end <= m_end)
    return true;

  return false;
}

std::string Interval::toStdString() const
{
  std::string output;
  
  if(m_start == m_end)
  {
    output += boost::lexical_cast<std::string>(m_start);
  }
  else
  {
    output += boost::lexical_cast<std::string>(m_start) + "-";
    output += boost::lexical_cast<std::string>(m_end);
  }

  return output;
}

QString Interval::toQString() const
{
  QString output;
  
  if(m_start == m_end)
  {
    output.append(QString("%1").arg(m_start));
  }
  else
  {
    output.append(QString("%1").arg(m_start)); 
    output += "-";
    output.append(QString("%1").arg(m_end));
  }

  return output;
}

//----------------------------------
// Interval private methods
//----------------------------------
void Interval::init(int start, int end)
{
  if(start <= end)
  {
    m_start = start;
    m_end = end;
  }
  // Here we cater for the case where a user sets start to be at say 4 but
  // end at 2.  We redefine the interval to be "2-4".
  else
  {
    m_start = end;
    m_end = start;
  }
}

//----------------------------------
// IntervalList public methods
//----------------------------------
IntervalList::IntervalList(void)
{

}

IntervalList::IntervalList(QString intervals)
{
  addIntervals(intervals);
}

IntervalList::IntervalList(Interval interval)
{
  m_list.append(interval);
}

IntervalList::IntervalList(const IntervalList& copy)
{
  m_list = copy.m_list;
}

const QList<Interval>& IntervalList::getList() const
{
  return m_list;
}

int IntervalList::totalIntervalLength() const
{
  // Total up all the individual Interval lengths in the list:
  
  int total = 0;

  for(int i = 0; i < m_list.size(); i++)
  {
    total += (m_list.at(i).length());
  }

  return total;
}

std::string IntervalList::toStdString(int numOfIntervals) const
{
  std::string output;

  if(m_list.size() <= numOfIntervals)
  {
    for(int i = 0; i < m_list.size(); i++)
    {
      if(i > 0) output += ", ";

      output += m_list.at(i).toStdString();
    }
  }
  // If the number of Intervals is over the numOfIntervals, then
  // we only print out the first (numOfIntervals - 1) Intervals,
  // followed by a ", ...", followed by the final Interval.
  // E.g. "0,2,4,6,8,10,12,14,16,18" becomes "0,2,4,6,8,...,18"
  else
  {
    for(int i = 0; i < numOfIntervals - 1; i++)
    {
      if(i > 0) output += ", ";

      output += m_list[i].toStdString();
    }

    output += ", ..., ";
    output += m_list[m_list.size() - 1].toStdString();
  }
  return output;
}

QString IntervalList::toQString(int numOfIntervals) const
{
  QString output(toStdString(numOfIntervals).c_str());

  return output;
}

void IntervalList::addInterval(int single)
{
  Interval interval(single, single);

  IntervalList::addInterval(interval);
}

// Note: this is considerably more efficient in the case where intervals are added
// smallest first.
void IntervalList::addInterval(Interval interval)
{
  if(m_list.size() == 0)
  {
    m_list.append(interval);
    return;
  }
  
  bool added = false;
  QList<int> deleteList;

  for(int i = m_list.size() - 1; i >= 0 ; i--)
  {
    // if new interval is completely higher than this interval
    if(interval.start() > m_list.at(i).end() + 1)
    {
      // add new interval as a seperate interval
      m_list.append(interval);
      added = true;
      break;
    }
    // else if the new interval can be merged with this interval
    else if(m_list.at(i).canMerge(interval))
    {
      // for each interval in the list before and including this one
      for(int j = i; j >= 0; j--)
      {
        // if it can be merged into the new interval
        if(m_list.at(j).canMerge(interval))
        {
          // do it
          interval.merge(m_list.at(j));
          // then add its index to the list of intervals to be deleted
          deleteList.append(j);
        }
        // else if it cant, there is no need to continue checking whether 
        // any other intervals can alse be merged
        else
        {
          break;
        }
      }
      // insert the new large interval in the correct place
      m_list.insert(i+1, interval);
      added = true;
      break;
    }
  }
  // if deleteList has any elements, delete intervals at those indices
  if(deleteList.size() > 0)
  {
    qSort(deleteList);
      
    for(int i = deleteList.size() - 1; i >=0 ; i--)
    {
      m_list.removeAt(deleteList[i]);
    }
  }
  // if still not assigned, add to the beginning 
  if(!added)
  {
    m_list.insert(0, interval);
  }
}

void IntervalList::addInterval(int start, int end)
{
  Interval interval(start, end);

  IntervalList::addInterval(interval);
}

void IntervalList::addIntervals(QString intervals)
{
  // Remove whitespace
  intervals = intervals.simplified();
  intervals = intervals.replace(" ", "");
  
  // Split the string, and add the intervals to the list.
  QStringList intervalList = intervals.split(",");
  for(int i = 0; i < intervalList.size(); i++)
  {
    Interval interval(intervalList[i]);
    addInterval(interval);
  }
}

void IntervalList::addIntervalList(const IntervalList& intervals)
{
  const QList<Interval> list = intervals.getList();

  QList<Interval>::const_iterator it = list.constBegin();

  for( ; it != list.constEnd(); ++it)
  {
    addInterval((*it));
  }
}

void IntervalList::setIntervalList(const IntervalList& intervals)
{
  m_list = QList<Interval>(intervals.getList());
}

std::set<int> IntervalList::getIntSet() const
{
  std::set<int> intSet;

  for(int i = 0; i < m_list.size(); i++)
  {
    std::set<int> intervalSet = m_list.at(i).getIntSet();
    intSet.insert(intervalSet.begin(), intervalSet.end());
  }

  return intSet;
}

bool IntervalList::contains(const Interval& other) const
{
  for(int i = 0; i < m_list.size(); i++)
  {
    if(m_list.at(i).contains(other))
      return true;
  }

  return false;
}

bool IntervalList::contains(const IntervalList& other) const
{
  for(int i = 0; i < other.m_list.size(); i++)
  {
    if(!IntervalList::contains(other.m_list.at(i)))
      return false;
  }

  return true;
}

bool IntervalList::isParsable(const QString &input, const IntervalList &container)
{
  try
  {
    const IntervalList test(input);
    return container.contains(test);
  } 
  catch (std::exception&)
  {
    return false;
  }
}

bool IntervalList::isParsable(const QString &input)
{
  try
  {
    IntervalList interval(input);
    return true;
  } 
  catch (std::exception&)
  {
    return false;
  }
}

IntervalList IntervalList::intersect(const IntervalList& aList, const Interval& bInterval)
{
  const IntervalList bList(bInterval);

  return IntervalList::intersect(aList, bList);
}

IntervalList IntervalList::intersect(const IntervalList& a, const IntervalList& b)
{
  IntervalList output;

  const std::set<int> aInts = a.getIntSet();
  const std::set<int> bInts = b.getIntSet();

  for( auto aInt = aInts.begin(); aInt != aInts.end(); ++aInt )
  {
    const bool inIntervalListB = bInts.find(*aInt) != bInts.end();
    if( inIntervalListB )
      output.addInterval(*aInt);
  }

  return output;
}

//----------------------------------
// IntervalListValidator public methods
//----------------------------------
IntervalListValidator::IntervalListValidator(QObject * parent, const IntervalList &intervalList) : QValidator(parent), m_intervalList(intervalList)
{
}

QValidator::State IntervalListValidator::validate(QString &input, int &pos) const
{
  UNUSED_ARG(pos)
  if(IntervalList::isParsable(input, m_intervalList))
    return QValidator::Acceptable;
  
  const QString pattern("^(\\d|-|,)*$");
  const QRegExp regExp(pattern);
  
  if(regExp.exactMatch(input))
    return QValidator::Intermediate;
  
  return QValidator::Invalid;
}



//////////////////////////////////////
// QLineEditWithErrorMark
//////////////////////////////////////
MantidWSIndexDialog::QLineEditWithErrorMark::QLineEditWithErrorMark(QWidget * parent): QWidget(parent){
  QGridLayout * layout = new QGridLayout(); 
  _lineEdit = new QLineEdit(); 
  m_validLbl = new QLabel("*"); // make it red
  QPalette pal = m_validLbl->palette(); 
  pal.setColor(QPalette::WindowText, Qt::darkRed); 
  m_validLbl->setPalette(pal); 
  layout->addWidget(_lineEdit,0,0); 
  layout->addWidget(m_validLbl,0,1);
  m_validLbl->setVisible(false);
  setLayout(layout);
}

void MantidWSIndexDialog::QLineEditWithErrorMark::setError(QString error){
  if(error.isEmpty()){
    m_validLbl->setVisible(false); 
  }else{
    m_validLbl->setVisible(true); 
    m_validLbl->setToolTip(error.trimmed()); 
  }
}
