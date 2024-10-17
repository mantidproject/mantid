.. _mpl_in_cpp:

=================
Matplotlib in C++
=================

.. contents::
  :local:


Overview
--------

This document describes the functionality with the C++ layer for interacting with matplotlib plotting.

Embedding Figures
-----------------

Matplotlib is designed such that figures can be embedded within other Qt widgets and not just created as separate
figure windows.
An example of how this can be achieved is described in the matplotlib
`examples <https://matplotlib.org/stable/gallery/user_interfaces/embedding_in_qt_sgskip.html>`_ section for a pure-Python
based implementation.

Many of our scientific interfaces are written in C++ and require the ability to embed a matplotlib figure within them.
Mantid contains a library, ``mplcpp``, to allow this interaction to occur without every developer having to be
concerned with the Python/C++ translation.

The main work is done by the ``FigureCanvasQt`` class and the ``Axes`` class. These are designed to closely mimic the
matplotlib classes in the methods that they offer.

Plotting from Vector Data
#########################

As a simple example consider a widget that wishes to display a single figure with data from two vectors:

.. code-block:: cpp

   // ------------- .h file ------------------
   #include "MantidQtWidgets/MplCpp/Line2D.h"
   #include <QVBoxLayout>

   namespace MantidQt {
   namespace Widgets { namespace MplCpp {
     class FigureCanvasQt;
   }}

   namespace MyWidgets {

   class WidgetWithFigure : public QWidget {
     Q_OBJECT

   public:
     WidgetWithFigure(QWidget *parent=nullptr);


   private:
      void initLayout();
      void doPlot();

      FigureCanvasQt *m_canvas;
      Line2D m_line;
   };

   // ------------- .cpp file ------------------
   #include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

   WidgetWithFigure::WidgetWithFigure(QWidget *parent=nullptr) : QWidget(parent),
     /// 111 is a standard matplotlib subplot specification
     m_canvas{new FigureCanvasQt(111, this)} {
       initLayout();
       doPlot();
   }

   void WidgetWithFigure::initLayout() {
       auto plotLayout = new QVBoxLayout(this);
       plotLayout->addWidget(m_canvas);
   }

   void WidgetWithFigure::doPlot() {
     const std::vector<double> xpoints{1, 2, 3, 4, 5}, ypoints{1, 2, 3, 4, 5};

     // In this mode make sure to capture the returned object as it owns the data
     // and when the Line2D is destroyed it will be removed from the canvas.
     m_line = m_canvas->gca().plot(xpoints, ypoints);
     // Most operations on the canvas don't automatically redraw the canvas
     // so drawing must be forced
     m_canvas->draw();
   }

The most important part of the above example is capturing the returned ``Line2D`` object.
In this basic mode with vectors the library avoids copying the data vectors but this means that
something needs to keep the data alive while it is displayed.

Plotting from Workspace Data
############################

A common case will be plotting data from a ``MatrixWorkspace``. In this case we leverage the ``mantid``
projection in a similar manner to a :ref:`standard script <plotting>`:

.. code-block:: cpp

   // ------------- .h file ------------------
   #include <QVBoxLayout>

   namespace MantidQt {
   namespace Widgets { namespace MplCpp {
     class FigureCanvasQt;
   }}

   namespace MyWidgets {

   class WidgetWithFigure : public QWidget {
     Q_OBJECT

   public:
     WidgetWithFigure(QWidget *parent=nullptr);


   private:
      void initLayout();
      void doPlot();

      FigureCanvasQt *m_canvas;
   };

   // ------------- .cpp file ------------------
   #include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
   #include "MantidQtWidgets/MplCpp/MantidAxes.h"

   using MantidQt::Widgets::MplCpp::MantidAxes;

   namespace {
     constexpr auto PROJECTION = "mantid";
   }


   WidgetWithFigure::WidgetWithFigure(QWidget *parent=nullptr) : QWidget(parent),
     /// 111 is a standard matplotlib subplot specification
     m_canvas{new FigureCanvasQt(111, PROJECTION, this)} {
       initLayout();
       doPlot();
   }

   void WidgetWithFigure::initLayout() {
       auto plotLayout = new QVBoxLayout(this);
       plotLayout->addWidget(m_canvas);
   }

   void WidgetWithFigure::doPlot() {
     const auto ws = getMyWorkspaceFromSomewhere();

     // In this mode the data is copied to matplotib so there is no need to keep
     // hold of the returned object
     m_line = m_canvas->gca<MantidAxes>().plot(ws, 0);
     // Most operations on the canvas don't automatically redraw the canvas
     // so drawing must be forced
     m_canvas->draw();
   }

In this mode the data does not need to be held by the C++ object as it is copied to the matplotlib curve.
