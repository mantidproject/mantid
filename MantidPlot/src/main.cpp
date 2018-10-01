/***************************************************************************
    File                 : main.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : QtiPlot main function

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include <QApplication>
#include <QBitmap>
#include <QDir>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSplashScreen>
#include <QThread>
#include <QTimer>
#include <iostream>

#include "ApplicationWindow.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "Mantid/MantidApplication.h"

// The following stuff is for the doxygen title page
/**  \mainpage QtiPlot - Data analysis and scientific plotting - API
documentation

  \section description Program description:
Scientists often need to use data analysis and plotting software.
For Windows systems there is a well known and widely used software
called <a href="http://www.originlab.com">Origin</a>, which is not
free, of course. The purpose of this project is to develop
a free (open source), platform independent alternative to
Origin. QtiPlot is still far from this goal, but
there's a <a class="el"
href="http://soft.proindependent.com/wishtolist.html">"wish to"</a> list we are
constantly working on.
All suggestions and contributions are most welcome!
If you want to contribute code, please read the notes on \ref style "coding
style" first.
<br>

 The QtiPlot web page can be found at<br>
 <a
href="http://soft.proindependent.com/qtiplot.html">http://soft.proindependent.com/qtiplot.html</a><br>

  \section libs QtiPlot uses the following libraries:
  <a href="http://www.trolltech.com/products/qt/index.html">Qt</a>,
  <a href="http://qwt.sourceforge.net/index.html">Qwt (5.0.0)</a>,
  <a href="http://qwtplot3d.sourceforge.net/">QwtPlot3D</a>,
  <a href="http://sources.redhat.com/gsl/">GSL</a>,
  <a href="http://muparser.sourceforge.net/">muParser</a>,
  <a href="http://www.zlib.net/">zlib (1.2.3)</a>,
  and <a href="http://sourceforge.net/projects/liborigin/">liborigin</a>.
  <br>

  \page future Future Plans

  - Make QtiPlot easier to extend and maintain by modularizing it. For
    example, ideally it should be possible to implement new plot or marker
    types without modifying existing classes (think of plug-ins here).
  - Unify naming of some methods and variables with class names.
  - Add generic plug-in support, support for implementing extensions in
    Python, a full-featured multi-level undo/redo framework (-&gt; visitor
pattern).
         [ undo/redo assigned to thzs; plug-in/Python extension support assigned
to knut ]
  - Revise some internal APIs that use undocumented protocols for the
    strings that get passed around. Wherever possible, consistency should be
    checked at compile-time. This would also probably eliminate the need to
    place restrictions on object/column names.
  - Switch file format to a completely XML-based one so we can write a
    more robust parser based on Qt's XML support. Legacy support for the old
    format could be a plug-in.
         See whether we can make use of the XML save/restore interface planned
for Qwt.
         Wait for the new XML reader in Qt 4.3 or use SAX?
  - Document as much as possible using doxygen and write simple demo
    plug-ins.
  - Modularize the build process so that Python support could use SIP's
    build system instead of the current hacks. Support for muParser,
    liborigin and zlib could also be split out into plug-ins on the basis
    that you shouldn't have to install libraries you don't use. Also reduces
    compile times during bugfixing.
         [ assigned to knut ]
  - Change some internal APIs that depend on the notion of a selected/current
item.
    This will lead to a cleaner and simpler interface for plug-ins and Python
scripts.
  .

  %Note about modularization: this is mainly about internal reorganizations.
  Most of the current features should remain part of the main executable, but
use interfaces similar or
  identical to those used by plug-ins. This should ease maintenance and make
adding new features
  to the core application a no-brainer once they're available as plug-ins.
  Support for Python, liborigin and zlib could be real, external plug-ins since
many people probably
  don't need them, but this can be decided shortly before distribution and
ideally by
  modifying just a few lines in a build script.

  \section features Features
  - Waterfall plots
  - Polar charts
  - Ternary plots
  - Improved data analysis: base-line detection tool, more built-in fit
functions...
  - Improved import of Origin project files
  - More import/export filters including Open Document spreadsheets (plugin)

  \sa ApplicationWindow, MultiLayer, Graph, Graph3D, ImageMarker, Legend,
MyWidget, SelectionMoveResizer, MyParser, Note, ScriptEdit, Table

  \page style Coding Style
  The following rules are not used everywhere (yet), but are intended as
guidelines for new code and eventually
  old code should be adapted as well.

  - Files use Unix-style line endings ("\n").
  - Class names start with a capital letter, object/variable names with a lower
case letter.
  - Methods and functions are named likeThis(), variables like_this and instance
variables are prefixed with "d_".
  - Property access methods use Qt style: property() and setProperty().
  - Every file should be named exactly like the class inside and there should be
only one class per file, with the exception of
    really short classes like LayerButton.
  - For indentations, tabs are preferred because they allow everyone to choose
the indentation depth for him/herself.
*/

int main(int argc, char **argv) {
  // First, look for command-line arguments that we want to deal with before
  // launching anything
  if (argc == 2) {
    QString str(argv[1]);
    if (str == "-v" || str == "--version") {
      std::cout << Mantid::Kernel::MantidVersion::version() << " ("
                << Mantid::Kernel::MantidVersion::releaseDate() << ")\n";
      exit(0);
    } else if (str == "-r" ||
               str == "--revision") // Print abbreviated git SHA-1
    {
      QString revision(Mantid::Kernel::MantidVersion::revision());
      std::cout << revision.toStdString() << '\n';
      exit(0);
    } else if (str == "-a" || str == "--about") {
      MantidApplication app(argc, argv); // Needed to avoid an error
      ApplicationWindow::about();
      exit(0);
    } else if (str == "-h" || str == "--help") {
      std::string s = "\nUsage: ";
      s += "MantidPlot [options] [filename]\n\n";
      s += "Valid options are:\n";
      s += "-a or --about: show about dialog and exit\n";
      s += "-d or --default-settings: start MantidPlot with the default "
           "settings\n";
      s += "-h or --help: show command line options\n";
      s += "-v or --version: print MantidPlot version and release date\n";
      s += "-r or --revision: print MantidPlot version and release date\n";
      s += "-s or --silent: start mantidplot without any setup dialogs\n";
      s += "-x or --execute: execute the script file given as argument\n";
      s += "-xq or --executeandquit: execute the script file given as argument "
           "and then exit MantidPlot\n\n";
      s += "'filename' can be any of .qti, qti.gz, .opj, .ogm, .ogw, .ogg, .py "
           "or ASCII file\n\n";
      std::cout << s;

      exit(0);
    }
  }

  MantidApplication app(argc, argv);

  QStringList args = app.arguments();
  args.removeFirst(); // remove application name

  try {
    // Splash
    QPixmap pixmap;
    if (!pixmap.load(":/MantidSplashScreen.png"))
      std::cerr << "Couldn't load splashscreen\n";
    QSplashScreen splash(pixmap);
    const QString releaseDateTime(Mantid::Kernel::MantidVersion::releaseDate());
    const QString versionInfo(Mantid::Kernel::MantidVersion::version());
    splash.showMessage("Release: " + releaseDateTime + " (Version " +
                           versionInfo + ")",
                       Qt::AlignLeft | Qt::AlignBottom);
    splash.setMask(pixmap.createMaskFromColor(QColor(Qt::transparent)));
    splash.show();
    // If we take too long to get to the event loop then box starts out gray so
    // ensure
    // it is painted before doing any heavy lifting like the ApplicationWindow
    // init
    splash.repaint();
    app.processEvents();

    const bool factorySettings =
        (args.contains("-d") || args.contains("--default-settings"));
    ApplicationWindow *mw = new ApplicationWindow(factorySettings, args);
    mw->restoreApplicationGeometry();
    mw->parseCommandLineArguments(args);
    QObject::connect(&app, SIGNAL(runAsPythonScript(const QString &)), mw,
                     SLOT(runPythonScript(const QString &)),
                     Qt::DirectConnection);
    app.processEvents();

    // register a couple of fonts
    QFontDatabase::addApplicationFont(
        ":/fonts/MontserratAlternates-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");

    splash.finish(mw);

    app.connect(&app, SIGNAL(lastWindowClosed()), mw,
                SLOT(exitWithPresetCode()));
    QTimer::singleShot(0, mw, SLOT(onAboutToStart()));
    return app.exec();
  } catch (std::exception &e) {
    QMessageBox::critical(
        nullptr, "Mantid - Error",
        QString("An unhandled exception has been caught. MantidPlot will have "
                "to close. Details:\n\n") +
            e.what());
  } catch (...) {
    QMessageBox::critical(nullptr, "Mantid - Error",
                          "An unhandled exception has "
                          "been caught. MantidPlot will "
                          "have to close.");
  }
}
