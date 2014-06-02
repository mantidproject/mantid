#include "TiledWindow.h"
#include "ApplicationWindow.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QSignalMapper>
#include <QActionGroup>
#include <QApplication>

// constants defining the minimum size of tiles
const int minimumTileWidth = 100;
const int minimumTileHeight = 100;
// tile border constants
const QColor normalColor("black");
const QColor selectedColor("green");
const QColor acceptDropColor("red");
const int normalWidth(1);
const int selectedWidth(5);
const int acceptDropWidth(5);

/**
 * Constructor.
 */
Tile::Tile(QWidget *parent):
  QFrame(parent),
  m_tiledWindow(parent),
  m_widget(NULL),
  m_selected(false),
  m_acceptDrop(false)
  //m_border(normalColor),
  //m_borderWidth(normalWidth)
{
  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(5,5,5,5);
}

/**
 * Destructor.
 */
Tile::~Tile()
{
  //std::cerr << "Tile deleted." << std::endl;
}

/**
 * Set a widget to this tile.
 * @param w :: A widget to set.
 */
void Tile::setWidget(MdiSubWindow *w)
{
  if (w == NULL)
  {
    removeWidget();
    return;
  }

  // widget cannot be replaced
  if (m_widget)
  {
    throw std::runtime_error("Widget already set");
  }

  m_layout->addWidget(w);
  m_widget = w;
}

/**
 * Remove attached widget.
 */
void Tile::removeWidget()
{
  m_layout->takeAt(0);
  // m_widget needs it's parent set or bad things happen
  m_widget->setParent( m_tiledWindow );
  m_widget = NULL;
}

/**
 */
void Tile::paintEvent(QPaintEvent *ev)
{
  QPainter p(this);
  if ( m_acceptDrop )
  {
    QPen pen(acceptDropColor);
    pen.setWidth(acceptDropWidth);
    p.setPen(pen);
  }
  else if ( m_selected )
  {
    QPen pen(selectedColor);
    pen.setWidth(selectedWidth);
    p.setPen(pen);
  }
  else
  {
    QPen pen(normalColor);
    pen.setWidth(normalWidth);
    p.setPen(pen);
  }
  p.drawRect( this->rect().adjusted(0,0,-1,-1) );
  QFrame::paintEvent(ev);
}

/**
 * Make this tile look selected or deselected: change the border colour.
 */
void Tile::makeSelected(bool yes)
{
  m_selected = yes;
  update();
}

/**
 * Make this tile show that it accepts widget drops or not by changing
 * border width and colour.
 */
void Tile::makeAcceptDrop(bool yes)
{
  m_acceptDrop = yes;
  update();
}

/**
 * Constructor.
 * @param parent :: The parent widget.
 * @param label :: The label.
 * @param name :: The name of the window.
 * @param f :: Window flags.
 */
TiledWindow::TiledWindow(QWidget* parent, const QString& label, const QString& name, Qt::WFlags f)
  : MdiSubWindow(parent, label, name, f),m_scrollArea(NULL),m_layout(NULL)
{
  init();
  setGeometry(0,0,500,400);
  setAcceptDrops( true );
}

void TiledWindow::init()
{
  delete m_scrollArea;
  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setWidgetResizable(true);

  QWidget *innerWidget = new QWidget(m_scrollArea);
  m_layout = new QGridLayout(innerWidget);
  m_layout->setMargin(6);
  m_layout->setColumnMinimumWidth(0,minimumTileWidth);
  m_layout->setRowMinimumHeight(0,minimumTileHeight);
  m_layout->addWidget(new Tile(this), 0, 0);
  m_layout->setColumnMinimumWidth(0,minimumTileWidth);
  m_layout->setRowMinimumHeight(0,minimumTileHeight);
  m_layout->setColStretch(0,1);

  m_scrollArea->setWidget(innerWidget);
  this->setWidget( NULL );
  this->setWidget( m_scrollArea );

}

QString TiledWindow::saveToString(const QString &info, bool)
{
  UNUSED_ARG(info);
  QString s= "<tiled_widget>\n";
  s+="</tiled_widget>\n";
  return s;
}

void TiledWindow::restore(const QStringList& data)
{
  UNUSED_ARG(data);
}

void TiledWindow::print()
{
}

/**
 * Get number of rows
 */
int TiledWindow::rowCount() const
{
  return m_layout->rowCount();
}

/**
 * Get number of columns
 */
int TiledWindow::columnCount() const
{
  return m_layout->columnCount();
}

/**
 * Remove all widgets.
 */
void TiledWindow::clear()
{
  init();
}

/**
 * Re-arrange the tiles according to a new layout shape.
 * @param newColumnCount :: A new number of columns in the layout.
 */
void TiledWindow::reshape(int newColumnCount)
{
  int nrows = rowCount();
  int ncols = columnCount();

  // remove all widgets and store their pointers
  QList<MdiSubWindow*> widgets;
  for(int row = 0; row < nrows; ++row)
  {
    for(int col = 0; col < ncols; ++col)
    {
      Tile *tile = getTile(row,col);
      MdiSubWindow *widget = tile->widget();
      if ( widget != NULL )
      {
        tile->removeWidget();
        widgets.append( widget );
      }
    }
  }

  int nWidgets = widgets.size();
  if ( nWidgets < newColumnCount )
  {
    newColumnCount = nWidgets;
  }

  if ( newColumnCount == 0 ) return;

  // clear the layout
  init();
  // make sure new dimensions will fit all widgets
  int newRowCount = nWidgets / newColumnCount;
  if ( newRowCount * newColumnCount != nWidgets ) 
  {
    newRowCount += 1;
  }
  // set up the new layout by putting in an empty tile at the top-right corner
  // m_layout now knows its rowCount and columnCount and calcTilePosition can be used
  Tile *tile = getOrAddTile(newRowCount - 1, newColumnCount - 1);
  (void) tile;
  // re-insert the widgets
  for(int i = 0; i < nWidgets; ++i)
  {
    int row(0), col(0);
    calcTilePosition( i, row, col );
    addWidget( widgets[i], row, col );
  }
}

/**
 * Get a Tile widget at position(row,col). If it doesn't exist create it.
 * @param row :: The row of the cell.
 * @param col :: The column of the cell.
 */
Tile *TiledWindow::getOrAddTile(int row, int col)
{
  auto item = m_layout->itemAtPosition( row, col );
  if ( item == NULL )
  {
    m_layout->addWidget( new Tile(this), row, col );
    tileEmptyCells();
    item = m_layout->itemAtPosition( row, col );
    if ( item == NULL )
    {
      throw std::logic_error("TiledWindow cannot be properly initialized.");
    }
  }
  Tile *widget = dynamic_cast<Tile*>(item->widget());
  if ( widget != NULL ) return widget;

  throw std::logic_error("TiledWindow wasn't properly initialized.");
}

/**
 * If a cell contains an Tile return it. If the cell contains a widget of another type
 * throw an exception. If it's empty return fill it with a Tile and return it.
 * @param row :: The row of the cell.
 * @param col :: The column of the cell.
 */
Tile *TiledWindow::getTile(int row, int col)
{
  auto item = m_layout->itemAtPosition( row, col );
  if ( item == NULL )
  {
    throw std::runtime_error("Tile indices are out of range.");
  }
  Tile *widget = dynamic_cast<Tile*>(item->widget());
  if ( widget != NULL ) return widget;

  throw std::logic_error("TiledWindow wasn't properly initialized.");
}

/**
 * Tile empty cells with Tiles.
 */
void TiledWindow::tileEmptyCells()
{
  int nrows = rowCount();
  int ncols = columnCount();
  for(int row = 0; row < nrows; ++row)
  {
    for(int col = 0; col < ncols; ++col)
    {
      QLayoutItem *item = m_layout->itemAtPosition( row, col );
      if ( item == NULL )
      {
        m_layout->addWidget( new Tile(this), row, col );
      }
    }
  }
}

/**
 * Add a new sub-window at a given position in the layout.
 * The row and column indices do not have to be within the current shape - 
 * it will change accordingly.
 * @param widget :: An MdiSubWindow to add.
 * @param row :: A row index at which to place the new tile.
 * @param col :: A column index at which to place the new tile.
 */
void TiledWindow::addWidget(MdiSubWindow *widget, int row, int col)
{
  try
  {
    Tile *tile = getOrAddTile( row, col );
    // prepare the cell
    m_layout->setColumnMinimumWidth(col,minimumTileWidth);
    m_layout->setRowMinimumHeight(row,minimumTileHeight);
    m_layout->setColStretch(col,1);
    // detach the widget from ApplicationWindow
    widget->detach();
    // widget must have it's parent
    widget->setParent(this);
    // disable mouse events
    widget->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    // attach it to this window
    tile->setWidget(widget);
    // fill possible empty spaces with Tiles
    tileEmptyCells();
  }
  catch(std::invalid_argument& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot add a widget to a TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Get a widget at a position in the layout.
 * @param row :: The row of the tile
 * @param col :: The column of the tile
 * @return :: A pointer to the MdiSubWindow at this position or NULL if the tile is empty.
 */
MdiSubWindow *TiledWindow::getWidget(int row, int col) 
{
  Tile *tile = getTile(row,col);
  return tile->widget();
}

/**
 * Make a widget floating or docked.
 * @param w :: A MdiSubWindow. It must be already detached from the tile and ready to become floating or docked.
 * @param to :: A wrapper window option: Floating, Docked or Default.
 */
void TiledWindow::sendWidgetTo(MdiSubWindow *w, TiledWindow::RemoveDestination to)
{
  w->resizeToDefault();
  switch (to)
  {
  case( Floating ): w->undock(); break;
  case( Docked ):   w->dock(); break;
  case( Default ):  
  default:
    if ( applicationWindow()->isDefaultFloating( w ) )
    {
      w->undock();
    }
    else
    {
      w->dock();
    }
  }
}

/**
 * Take a widget at position (row,col), remove it and make docked.
 * @param row :: The tile's row index.
 * @param col :: The tile's column index.
 */
void TiledWindow::removeWidgetTo(int row, int col, TiledWindow::RemoveDestination to)
{
  try
  {
    deselectWidget( row, col );
    MdiSubWindow *widget = removeTile( row, col );
    sendWidgetTo( widget, to );
  }
  catch(std::runtime_error& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot remove a widget from a TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Take a widget at position (row,col), remove it and make docked.
 * @param row :: The tile's row index.
 * @param col :: The tile's column index.
 */
void TiledWindow::removeWidgetToDocked(int row, int col)
{
  removeWidgetTo( row, col, Docked );
}

/**
 * Take a tile at position (row,col), remove it and make floating.
 * @param row :: The tile's row index.
 * @param col :: The tile's column index.
 */
void TiledWindow::removeWidgetToFloating(int row, int col)
{
  removeWidgetTo( row, col, Floating );
}

/**
 * Remove (but don't delete) a tile.
 * @param row :: The row of a tile to remove.
 * @param col :: The column of a tile to remove.
 * @return :: A pointer to the removed subwindow.
 */
MdiSubWindow *TiledWindow::removeTile(int row, int col)
{
  Tile *tile = getTile( row, col );
  MdiSubWindow *widget = removeTile( tile );
  if ( widget == NULL )
  {
    QString msg = QString("Cell (%1,%2) is empty.").arg(row).arg(col);
    throw std::runtime_error(msg.toStdString());
  }
  return widget;
}

/**
 * Remove (but don't delete) a tile.
 * @param tile :: A tile to remove.
 * @return :: A pointer to the removed subwindow.
 */
MdiSubWindow *TiledWindow::removeTile(Tile *tile)
{
  MdiSubWindow *widget = tile->widget();
  if ( widget != NULL )
  {
    tile->removeWidget();
    widget->setAttribute(Qt::WA_TransparentForMouseEvents,false);
  }
  return widget;
}

/**
 * Get a tile at a mouse position (in pixels).
 * @param pos :: Position of the mouse as returned by QMouseEvent
 * @return :: A pointer to the tile or NULL if clicked on an empty space.
 */
Tile *TiledWindow::getTileAtMousePos( const QPoint& pos ) 
{
  for(int i = 0; i < m_layout->count(); ++i)
  {
    auto *item = m_layout->itemAt(i);
    if ( item != NULL )
    {
      if ( !item->geometry().contains(pos) ) continue;
      QWidget *w = item->widget();
      if ( w != NULL )
      {
        auto *tile = dynamic_cast<Tile*>(w);
        if ( tile != NULL )
        {
          return tile;
        }
        else
        {
          return NULL;
        }
      }
      else
      {
        return NULL;
      }
    }
  }
  return NULL;
}

/**
 * Get a list of all tiles.
 */
QList<Tile*> TiledWindow::getAllTiles()
{
  QList<Tile*> tiles;
  int nrows = rowCount();
  int ncols = columnCount();
  for(int row = 0; row < nrows; ++row)
  {
    for(int col = 0; col < ncols; ++col)
    {
      tiles.append( getTile(row, col) );
    }
  }
  return tiles;
}

/**
 * Mouse press event handler.
 */
void TiledWindow::mousePressEvent(QMouseEvent *ev)
{
  clearDrops();
  auto tile = getTileAtMousePos( ev->pos() );
  if ( tile == NULL ) return;
  if ( (ev->modifiers() & Qt::ShiftModifier) != 0 )
  {
    addRangeToSelection( tile );
  }
  else if ( (ev->modifiers() & Qt::ControlModifier) != 0 )
  {
    addToSelection( tile, true );
  }
}

/**
 * Mouse release event handler.
 */
void TiledWindow::mouseReleaseEvent(QMouseEvent *ev)
{
  if ( ev->modifiers() == Qt::NoModifier )
  {
    auto tile = getTileAtMousePos( ev->pos() );
    if ( tile == NULL ) return;
    addToSelection( tile, false );
  }
}

/**
 * Mouse move event handler.
 */
void TiledWindow::mouseMoveEvent(QMouseEvent *ev)
{
  if (!hasSelection() || (ev->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
  {
    return;
  }

  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  mimeData->setObjectName("TiledWindow");
  mimeData->setText( name() );

  drag->setMimeData(mimeData);
  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void) dropAction;
}

/**
 * Add a tile to the selection.
 * @param tile :: A tile to add.
 * @param append :: If true the tile will be appended to the existing selection. If false 
 *   any previous selection will be deselected and replaced with tile.
 */
void TiledWindow::addToSelection(Tile *tile, bool append)
{
  if ( tile == NULL ) return;
  if ( tile->widget() == NULL ) return;
  if ( append )
  {
    if ( deselectTile( tile ) )
    {
      return;
    }
  }
  else
  {
    clearSelection();
  }
  m_selection.append( tile );
  tile->makeSelected(true);
}

/**
 * Add a range of tiles to the selection. One of the ends of tha range
 * is given by an already selected tile with the lowest flat index (see calcFlatIndex).
 * The other end is the tile in the argument.
 * @param tile :: A new end tile of the range.
 */
void TiledWindow::addRangeToSelection(Tile *tile)
{
  if ( m_selection.isEmpty() )
  {
    addToSelection( tile, false );
    return;
  }

  int ifirst = rowCount() * columnCount();
  int ilast = 0;
  foreach(Tile *selected, m_selection)
  {
    int index = calcFlatIndex( selected );
    if ( index < ifirst ) ifirst = index;
    if ( index > ilast ) ilast = index;
  }

  int index = calcFlatIndex( tile );
  if ( index == ilast ) return;

  if ( index < ifirst )
  {
    ilast = ifirst;
    ifirst = index;
  }
  else
  {
    ilast = index;
  }

  clearSelection();
  for(int i = ifirst; i <= ilast; ++i)
  {
    int row(0), col(0);
    calcTilePosition( i, row, col );
    Tile *tile = getTile( row, col );
    addToSelection( tile, true );
  }
}

/**
 * Clear the selection.
 */
void TiledWindow::clearSelection()
{
  foreach(Tile *tile, m_selection)
  {
    tile->makeSelected(false);
  }
  m_selection.clear();
}

/**
 * Deselect a tile. If the tile isn't selected do nothing. 
 * @param tile :: A tile to deselect.
 * @return true if the tile was deselected and false if it was not selected before the call.
 */
bool TiledWindow::deselectTile(Tile *tile)
{
  int index = m_selection.indexOf( tile );
  if ( index >= 0 )
  {
    m_selection.removeAt( index );
    tile->makeSelected(false);
    return true;
  }
  return false;
}

/**
 * Check if there are any selected tiles
 */
bool TiledWindow::hasSelection() const
{
  return ! m_selection.isEmpty();
}

/**
 * Calculate an index of a tile as if they were in a 1d list of concatenated rows.
 * QLayout::indexOf doesn't work here.
 * @param tile :: A tile to get the index for.
 */
int TiledWindow::calcFlatIndex(Tile *tile) const
{
  int indexInLayout = m_layout->indexOf( tile );
  int row(0), col(0), rowSpan(1), colSpan(1);
  m_layout->getItemPosition( indexInLayout, &row, &col, &rowSpan, &colSpan );
  return row * columnCount() + col;
}

/**
 * Calculate tile's row and column indices given it's flat index as returned 
 * by calcFlatIndex(...).
 * @param index :: The flat index of a tile.
 * @param row :: A reference to a variable to accept the row index value.
 * @param col :: A reference to a variable to accept the column index value.
 */
void TiledWindow::calcTilePosition( int index, int &row, int &col ) const
{
  if ( index < 0 )
  {
    throw std::runtime_error("Flat index in TiledWindow is outside range.");
  }
  int ncols = columnCount();
  row = index / ncols;
  if ( row >= rowCount() )
  {
    throw std::runtime_error("Flat index in TiledWindow is outside range.");
  }
  col = index - row * ncols;
  if ( col >= ncols )
  {
    throw std::runtime_error("Flat index in TiledWindow is outside range.");
  }
}

/**
 * Select a widget at position.
 * @param row :: A row.
 * @param col :: A column.
 */
void TiledWindow::selectWidget(int row, int col)
{
  try
  {
    addToSelection( getTile(row,col), false );
  }
  catch(std::runtime_error& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot select a widget in TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Deselect a widget at position.
 * @param row :: A row.
 * @param col :: A column.
 */
void TiledWindow::deselectWidget(int row, int col)
{
  try
  {
    deselectTile( getTile(row,col) );
  }
  catch(std::runtime_error&)
  {}
}

/**
 * Check if a widget is selected.
 * @param row :: A row.
 * @param col :: A column.
 */
bool TiledWindow::isSelected(int row, int col)
{
  try
  {
    Tile *tile = getTile(row,col);
    return m_selection.contains(tile);
  }
  catch(std::runtime_error&)
  {}
  return false;
}

/**
 * Select a range of Widgets.
 * @param row1 :: Row index of the start of the range.
 * @param col1 :: Column index of the start of the range.
 * @param row2 :: Row index of the end of the range.
 * @param col2 :: Column index of the end of the range.
 */
void TiledWindow::selectRange(int row1, int col1, int row2, int col2)
{
  try
  {
    addToSelection( getTile(row1,col1), false );
    addRangeToSelection( getTile(row2,col2) );
  }
  catch(std::runtime_error& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot select widgets in TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Remove the selection and make its widgets floating or docked
 * @param to :: A wrapper window option: Floating, Docked or Default.
 */
void TiledWindow::removeSelectionTo(TiledWindow::RemoveDestination to)
{
  foreach(Tile *tile, m_selection)
  {
    MdiSubWindow *widget = removeTile( tile );
    if ( widget == NULL )
    {
      throw std::logic_error("TiledWindow: Empty tile is found in slection.");
    }
    sendWidgetTo( widget, to );
  }
  clearSelection();
}

/**
 * Remove the selection and make all windows docked.
 */
void TiledWindow::removeSelectionToDocked()
{
  removeSelectionTo( Docked );
}

/**
 * Remove the selection and make all windows floating.
 */
void TiledWindow::removeSelectionToFloating()
{
  removeSelectionTo( Floating );
}

/**
 * Remove the selection and put them into separate windows. Each window
 * will be either floating or docked depending on the default setting for
 * a particular MdiSubwindow type.
 */
void TiledWindow::removeSelectionToDefaultWindowType()
{
  removeSelectionTo( Default );
}

/**
 * Populate a menu with actions.
 * @param menu :: A menu to populate.
 */
void TiledWindow::populateMenu(QMenu *menu)
{  
  QAction *actionToDocked = new QAction("Selection to Docked",menu);
  connect(actionToDocked,SIGNAL(triggered()),this,SLOT(removeSelectionToDocked()));
  menu->addAction( actionToDocked );

  QAction *actionToFloating = new QAction("Selection to Floating",menu);
  connect(actionToFloating,SIGNAL(triggered()),this,SLOT(removeSelectionToFloating()));
  menu->addAction( actionToFloating );

  // reshape actions
  {
    QSignalMapper *reshapeMapper = new QSignalMapper(this);
    connect(reshapeMapper,SIGNAL(mapped(int)),this,SLOT(reshape(int)));

    QActionGroup *reshapeActionGroup = new QActionGroup(this);
    const int nShapes = 9;
    for( int i = 1; i < nShapes; ++i )
    {
      QAction *action = new QAction(QString("%1").arg(i),menu);
      action->setCheckable(true);
      connect(action,SIGNAL(triggered()),reshapeMapper,SLOT(map()));
      reshapeMapper->setMapping(action,i);
      reshapeActionGroup->addAction(action);
    }
    QMenu *submenu = new QMenu("Reshape");
    submenu->addActions(reshapeActionGroup->actions());
    menu->addMenu( submenu );
  }

  menu->addSeparator();

  QAction *actionClear = new QAction("Clear",menu);
  connect(actionClear,SIGNAL(triggered()),this,SLOT(clear()));
  menu->addAction( actionClear );
}

/**
 * Check if a Tile can accept drops.
 * @param tile :: A tile to check.
 */
bool TiledWindow::canAcceptDrops(Tile *tile) const
{
  return tile->widget() == NULL;
}

/**
 * Display a position in the layout where a widget will be inserted
 * given approximate coords.
 *
 * @param x :: Approx x-coord in pixels where a widget will be dropped.
 * @param y :: Approx y-coord in pixels where a widget will be dropped.
 */
void TiledWindow::showInsertPosition( QPoint pos, bool global )
{
  clearDrops();
  if ( global )
  {
    pos = mapFromGlobal( pos );
  }
  Tile *tile = getTileAtMousePos( pos );
  if ( tile )
  {
    if ( canAcceptDrops(tile) )
    {
      tile->makeAcceptDrop( true );
    }
  }
}

/// Tell all tiles to not show that they accept widget drops.
void TiledWindow::clearDrops()
{
  auto tiles = getAllTiles();
  foreach(Tile* tile, tiles)
  {
    tile->makeAcceptDrop(false);
  }
}

/**
 * Try to drop a widget at a mouse position. Return true if succeeded.
 * 
 * @param w :: A widget to drop.
 * @param x :: Approx x-coord in pixels where a widget will be dropped.
 * @param y :: Approx y-coord in pixels where a widget will be dropped.
 */
bool TiledWindow::dropAtPosition( MdiSubWindow *w, QPoint pos, bool global )
{
  clearDrops();
  if ( global )
  {
    pos = mapFromGlobal( pos );
  }
  Tile *tile = getTileAtMousePos( pos );
  if ( tile && canAcceptDrops(tile) )
  {
    int index = calcFlatIndex( tile );
    int row = -1, col = -1;
    calcTilePosition( index, row, col );
    addWidget( w, row, col );
    return true;
  }
  return false;
}

void TiledWindow::dragEnterEvent(QDragEnterEvent* ev)
{
  auto mimeData = ev->mimeData();
  if ( mimeData->hasFormat("TiledWindowE") )
  {
    ev->accept();
  }
  else
  {
    ev->ignore();
  }
}

void TiledWindow::dragLeaveEvent()
{
  clearDrops();
}

void TiledWindow::dragMoveEvent(QDragMoveEvent* ev)
{
  showInsertPosition( ev->pos(), false );
}

void TiledWindow::dropEvent(QDropEvent* ev)
{
  auto mimeData = ev->mimeData();
  const char *ptr = mimeData->data("TiledWindowE").constData();
  MdiSubWindow *w = reinterpret_cast<MdiSubWindow*>( const_cast<char*>(ptr) );
  dropAtPosition( w, ev->pos(), false );
}
