#ifndef TiledWindow_H
#define TiledWindow_H

#include "MdiSubWindow.h"

class QGridLayout;

/**
 *
 *  A mdi sub-window that contains other sub-windows arranged in rows and columns.
 *  The child sub-windows can be selected, moved, deleted, dragged and dropped.
 *
 */
class TiledWindow: public MdiSubWindow
{
  Q_OBJECT

public:
  TiledWindow(QWidget* parent, const QString& label, const QString& name = QString(), Qt::WFlags f=0);
public slots:

  /// Get number of rows
  int rowCount() const;
  /// Get number of columns
  int columnCount() const;
  /// Re-shape the tile layout
  void reshape(int rows, int cols);
  /// Add a new tile
  void addTile(MdiSubWindow *tile, int row, int col);
  /// Add a new tile
  MdiSubWindow *getTile(int row, int col)const;
  /// Remove a tile and make it docked
  void removeTileToDocked(int row, int col);
  /// Remove a tile and make it floating
  void removeTileToFloating(int row, int col);

  QString saveToString(const QString &info, bool = false);
  void restore(const QStringList&);
  void print();

private:
  /// The layout arranging the tiles into a grid.
  QGridLayout *m_layout;
};

/**
 * A widget-placeholder showing an empty cell where a sub-window can be inserted.
 */
class EmptyTile: public QWidget
{
public:
  EmptyTile(QWidget *parent);
};

#endif // TiledWindow_H
