#include "TiledWindow.h"
#include "ApplicationWindow.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPainter>

// constants defining the minimum size of tiles
const int minimumTileWidth = 100;
const int minimumTileHeight = 100;
// tile border constants
const QColor normalColor("black");
const QColor selectedColor("green");
const int normalWidth(1);
const int selectedWidth(5);

/**
 * Constructor.
 */
Tile::Tile(QWidget *parent):
  QFrame(parent),m_widget(NULL),m_border(normalColor),m_borderWidth(normalWidth)
{
  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(5,5,5,5);
}

/**
 * Destructor.
 */
Tile::~Tile()
{
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
  m_widget = NULL;
}

/**
 */
void Tile::paintEvent(QPaintEvent *ev)
{
  QPainter p(this);
  QPen pen(m_border);
  pen.setWidth(m_borderWidth);
  p.setPen(pen);
  p.drawRect( this->rect().adjusted(0,0,-1,-1) );
  QFrame::paintEvent(ev);
}

/**
 * Make this tile look selected: change the border colour.
 */
void Tile::makeSelected()
{
  m_border = selectedColor;
  m_borderWidth = selectedWidth;
  update();
}

/**
 * Make this tile look unselected.
 */
void Tile::makeNormal()
{
  m_border = normalColor;
  m_borderWidth = normalWidth;
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
  : MdiSubWindow(parent, label, name, f)
{
  QScrollArea *scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);

  QWidget *innerWidget = new QWidget(scrollArea);
  m_layout = new QGridLayout();
  m_layout->setMargin(6);
  m_layout->setColumnMinimumWidth(0,minimumTileWidth);
  m_layout->setRowMinimumHeight(0,minimumTileHeight);
  m_layout->addWidget(new Tile(this), 0, 0);
  m_layout->setColumnMinimumWidth(0,minimumTileWidth);
  m_layout->setRowMinimumHeight(0,minimumTileHeight);
  m_layout->setColStretch(0,1);
  innerWidget->setLayout( m_layout );

  scrollArea->setWidget(innerWidget);
  this->setWidget( scrollArea );
  setGeometry(0,0,500,400);
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
 * Re-arrange the tiles according to a new layout shape.
 * @param rows :: A new number of rows in the layout.
 * @param cols :: A new number of columns in the layout.
 */
void TiledWindow::reshape(int rows, int cols)
{
  UNUSED_ARG(rows);
  UNUSED_ARG(cols);
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
    Tile *tile = getTile( row, col );
    if ( tile == NULL )
    {
      tile = new Tile(this);
      m_layout->addWidget( tile );
    }
    // prepare the cell
    m_layout->setColumnMinimumWidth(col,minimumTileWidth);
    m_layout->setRowMinimumHeight(row,minimumTileHeight);
    m_layout->setColStretch(col,1);
    // detach the widget from ApplicationWindow
    widget->detach();
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
 * Take a widget at position (row,col), remove it and make docked.
 * @param row :: The tile's row index.
 * @param col :: The tile's column index.
 */
void TiledWindow::removeWidgetToDocked(int row, int col)
{
  try
  {
    MdiSubWindow *widget = removeTile( row, col );
    widget->dock();
  }
  catch(std::runtime_error& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot remove a widget from a TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Take a tile at position (row,col), remove it and make floating.
 * @param row :: The tile's row index.
 * @param col :: The tile's column index.
 */
void TiledWindow::removeWidgetToFloating(int row, int col)
{
  try
  {
    MdiSubWindow *widget = removeTile( row, col );
    widget->undock();
  }
  catch(std::runtime_error& ex)
  {
    QMessageBox::critical(this,"MantidPlot- Error","Cannot remove a widget from a TiledWindow:\n\n" + QString::fromStdString(ex.what()));
  }
}

/**
 * Remove (but don't delete) a tile.
 * @param tile :: A tile to remove.
 * @return :: A pointer to the removed subwindow.
 */
MdiSubWindow *TiledWindow::removeTile(int row, int col)
{
  Tile *tile = getTile( row, col );
  MdiSubWindow *widget = tile->widget();
  if ( widget != NULL )
  {
    tile->removeWidget();
    widget->setAttribute(Qt::WA_TransparentForMouseEvents,false);
    return widget;
  }
  QString msg = QString("Cell (%1,%2) is empty.").arg(row).arg(col);
  throw std::runtime_error(msg.toStdString());
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
 * Mouse press event handler.
 */
void TiledWindow::mousePressEvent(QMouseEvent *ev)
{
  auto tile = getTileAtMousePos( ev->pos() );
  if ( tile == NULL ) return;
  if ( (ev->modifiers() & Qt::ShiftModifier) != 0 )
  {
    addRangeToSelection( tile );
  }
  else
  {
    bool append = (ev->modifiers() & Qt::ControlModifier) != 0;
    addToSelection( tile, append );
  }
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
    int index = m_selection.indexOf( tile );
    if ( index >= 0 )
    {
      m_selection.removeAt( index );
      tile->makeNormal();
      return;
    }
  }
  else
  {
    clearSelection();
  }
  m_selection.append( tile );
  tile->makeSelected();
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
  if ( index == ifirst || index == ilast ) return;

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
    tile->makeNormal();
  }
  m_selection.clear();
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
    throw std::range_error("Flat index in TiledWindow is outside range.");
  }
  int ncols = columnCount();
  row = index / ncols;
  if ( row >= rowCount() )
  {
    throw std::range_error("Flat index in TiledWindow is outside range.");
  }
  col = index - row * ncols;
  if ( col >= ncols )
  {
    throw std::range_error("Flat index in TiledWindow is outside range.");
  }
}
