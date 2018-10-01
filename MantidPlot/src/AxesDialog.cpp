/***************************************************************************
File                 : AxesDialog.cpp
Project              : QtiPlot
--------------------------------------------------------------------
Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
Description          : General plot options dialog

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
#include "AxesDialog.h"
#include "ApplicationWindow.h"
#include "ColorBox.h"
#include "ColorButton.h"
#include "Graph.h"
#include "Grid.h"
#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "MantidQtWidgets/LegacyQwt/qwt_compat.h"
#include "MyParser.h"
#include "Plot.h"
#include "ScaleDraw.h"
#include "Table.h"
#include "TextDialog.h"
#include "TextFormatButtons.h"
#include <cfloat>
#include <cmath>

#include "MantidKernel/Logger.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTimeEdit>
#include <QFontDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTimeEdit>
#include <QVector>

#include "MantidQtWidgets/LegacyQwt/ScaleEngine.h"
#include <qwt_plot.h>
#include <qwt_scale_widget.h>

/* XPM */
static const char *bottom_scl_xpm[] = {"36 38 75 1",
                                       " 	c None",
                                       ".	c #FFFFFF",
                                       "+	c #818181",
                                       "@	c #070707",
                                       "#	c #101010",
                                       "$	c #EEEEEE",
                                       "%	c #1D1D1D",
                                       "&	c #ACACAC",
                                       "*	c #151515",
                                       "=	c #121212",
                                       "-	c #929292",
                                       ";	c #242424",
                                       ">	c #D5D5D5",
                                       ",	c #C9C9C9",
                                       "'	c #2A2A2A",
                                       ")	c #5E5E5E",
                                       "!	c #000000",
                                       "~	c #C8C8C8",
                                       "{	c #D2D2D2",
                                       "]	c #090909",
                                       "^	c #060606",
                                       "/	c #FEFEFE",
                                       "(	c #FCFCFC",
                                       "_	c #F7F7F7",
                                       ":	c #CECECE",
                                       "<	c #444444",
                                       "[	c #E0E0E0",
                                       "}	c #464646",
                                       "|	c #E8E8E8",
                                       "1	c #282828",
                                       "2	c #D3D3D3",
                                       "3	c #D1D1D1",
                                       "4	c #A4A4A4",
                                       "5	c #484848",
                                       "6	c #EBEBEB",
                                       "7	c #7D7D7D",
                                       "8	c #050505",
                                       "9	c #7F7F7F",
                                       "0	c #FDFDFD",
                                       "a	c #7D6F6F",
                                       "b	c #F38C82",
                                       "c	c #E71E09",
                                       "d	c #E82612",
                                       "e	c #FDEFEE",
                                       "f	c #E9321F",
                                       "g	c #F7B3AC",
                                       "h	c #E82B17",
                                       "i	c #E82814",
                                       "j	c #F49C93",
                                       "k	c #EA3826",
                                       "l	c #FBD8D5",
                                       "m	c #F9CEC9",
                                       "n	c #EA3E2C",
                                       "o	c #EF6D5F",
                                       "p	c #E71803",
                                       "q	c #F9CDC8",
                                       "r	c #FAD6D2",
                                       "s	c #E7200B",
                                       "t	c #E71D08",
                                       "u	c #FEFCFC",
                                       "v	c #FEF7F7",
                                       "w	c #FAD2CE",
                                       "x	c #ED5546",
                                       "y	c #FCE2E0",
                                       "z	c #ED5748",
                                       "A	c #FCEAE8",
                                       "B	c #EA3C2A",
                                       "C	c #FAD7D3",
                                       "D	c #FAD5D1",
                                       "E	c #F6ACA5",
                                       "F	c #ED594A",
                                       "G	c #FDECEB",
                                       "H	c #F2897E",
                                       "I	c #E71C07",
                                       "J	c #F28B80",
                                       "....................................",
                                       "......+@#+......$%......&*=-........",
                                       "......;>,'......)!......%~{]........",
                                       "......^/(^......_!........:<........",
                                       "......^((^.......!.......[}|........",
                                       "......123;.......!......456.........",
                                       "......78^9.......!......*!!!........",
                                       "+@#+................................",
                                       ";>,'...(!0.......!.......!.....+@#+.",
                                       "^/(^...!!!!!!!!!!!!!!!!!!!!!!..;>,'.",
                                       "^((^...!aaaaaaaaaaaaaaaaaaaa!..^/(^.",
                                       "123;..!!aaaaaaaaaaaaaaaaaaaa!!.^((^.",
                                       "78^9...!aaaaaaaaaaaaaaaaaaaa!..123;.",
                                       ".......!aaaaaaaaaaaaaaaaaaaa!..78^9.",
                                       ".......!aaaaaaaaaaaaaaaaaaaa!.......",
                                       "$%.....!aaaaaaaaaaaaaaaaaaaa!.......",
                                       ")!.....!aaaaaaaaaaaaaaaaaaaa!..$%...",
                                       "_!.....!aaaaaaaaaaaaaaaaaaaa!..)!...",
                                       ".!....!!aaaaaaaaaaaaaaaaaaaa!!._!...",
                                       ".!.....!aaaaaaaaaaaaaaaaaaaa!...!...",
                                       ".!.....!aaaaaaaaaaaaaaaaaaaa!...!...",
                                       ".......!aaaaaaaaaaaaaaaaaaaa!...!...",
                                       "&*=-...!aaaaaaaaaaaaaaaaaaaa!.......",
                                       "%~{]...!aaaaaaaaaaaaaaaaaaaa!..&*=-.",
                                       "..:<...!aaaaaaaaaaaaaaaaaaaa!..%~{].",
                                       ".[}|..!!aaaaaaaaaaaaaaaaaaaa!!...:<.",
                                       "456....!aaaaaaaaaaaaaaaaaaaa!...[}|.",
                                       "*!!!...!aaaaaaaaaaaaaaaaaaaa!..456..",
                                       ".......!!!!!!!!!!!!!!!!!!!!!!..*!!!.",
                                       ".........!........!.......!.........",
                                       "....................................",
                                       "........bcdb.....ef......ghij.......",
                                       "........klmn.....op......fqrs.......",
                                       "........t/ut.....vp........wx.......",
                                       "........tuut......p.......yzA.......",
                                       "........BCDk......p......EFG........",
                                       "........HItJ......p......hppp.......",
                                       "...................................."};

/* XPM */
static const char *top_scl_xpm[] = {"36 38 75 1",
                                    " 	c None",
                                    ".	c #FFFFFF",
                                    "+	c #F38C82",
                                    "@	c #E71E09",
                                    "#	c #E82612",
                                    "$	c #FEFAFA",
                                    "%	c #E9321F",
                                    "&	c #F7B3AC",
                                    "*	c #E82B17",
                                    "=	c #E82814",
                                    "-	c #F49C93",
                                    ";	c #EA3826",
                                    ">	c #FBD8D5",
                                    ",	c #F9CEC9",
                                    "'	c #EA3E2C",
                                    ")	c #EF6D5F",
                                    "!	c #E71803",
                                    "~	c #F9CDC8",
                                    "{	c #FAD6D2",
                                    "]	c #E7200B",
                                    "^	c #E71D08",
                                    "/	c #FEFEFE",
                                    "(	c #FEFCFC",
                                    "_	c #FAD2CE",
                                    ":	c #ED5546",
                                    "<	c #FCE2E0",
                                    "[	c #ED5748",
                                    "}	c #FCEAE8",
                                    "|	c #EA3C2A",
                                    "1	c #FAD7D3",
                                    "2	c #FAD5D1",
                                    "3	c #F6ACA5",
                                    "4	c #ED594A",
                                    "5	c #FDF2F1",
                                    "6	c #F2897E",
                                    "7	c #E71C07",
                                    "8	c #F28B80",
                                    "9	c #818181",
                                    "0	c #070707",
                                    "a	c #101010",
                                    "b	c #242424",
                                    "c	c #D5D5D5",
                                    "d	c #C9C9C9",
                                    "e	c #2A2A2A",
                                    "f	c #000000",
                                    "g	c #060606",
                                    "h	c #FCFCFC",
                                    "i	c #7D6F6F",
                                    "j	c #282828",
                                    "k	c #D3D3D3",
                                    "l	c #D1D1D1",
                                    "m	c #7D7D7D",
                                    "n	c #050505",
                                    "o	c #7F7F7F",
                                    "p	c #FAFAFA",
                                    "q	c #1D1D1D",
                                    "r	c #5E5E5E",
                                    "s	c #ACACAC",
                                    "t	c #151515",
                                    "u	c #121212",
                                    "v	c #929292",
                                    "w	c #C8C8C8",
                                    "x	c #D2D2D2",
                                    "y	c #090909",
                                    "z	c #CECECE",
                                    "A	c #444444",
                                    "B	c #E0E0E0",
                                    "C	c #464646",
                                    "D	c #E8E8E8",
                                    "E	c #A4A4A4",
                                    "F	c #484848",
                                    "G	c #F1F1F1",
                                    "H	c #EEEEEE",
                                    "I	c #F7F7F7",
                                    "J	c #EBEBEB",
                                    "....................................",
                                    ".......+@#+.....$%......&*=-........",
                                    ".......;>,'.....)!......%~{]........",
                                    ".......^/(^......!........_:........",
                                    ".......^((^......!.......<[}........",
                                    ".......|12;......!......345.........",
                                    ".......67^8......!......*!!!........",
                                    "90a9................................",
                                    "bcde....f........f.......f.....90a9.",
                                    "g/hg...ffffffffffffffffffffff..bcde.",
                                    "ghhg...fiiiiiiiiiiiiiiiiiiiif..g/hg.",
                                    "jklb..ffiiiiiiiiiiiiiiiiiiiiff.ghhg.",
                                    "mngo...fiiiiiiiiiiiiiiiiiiiif..jklb.",
                                    ".......fiiiiiiiiiiiiiiiiiiiif..mngo.",
                                    ".......fiiiiiiiiiiiiiiiiiiiif.......",
                                    "pq.....fiiiiiiiiiiiiiiiiiiiif.......",
                                    "rf.....fiiiiiiiiiiiiiiiiiiiif..pq...",
                                    ".f.....fiiiiiiiiiiiiiiiiiiiif..rf...",
                                    ".f....ffiiiiiiiiiiiiiiiiiiiiff..f...",
                                    ".f.....fiiiiiiiiiiiiiiiiiiiif...f...",
                                    ".f.....fiiiiiiiiiiiiiiiiiiiif...f...",
                                    ".......fiiiiiiiiiiiiiiiiiiiif...f...",
                                    "stuv...fiiiiiiiiiiiiiiiiiiiif.......",
                                    "qwxy...fiiiiiiiiiiiiiiiiiiiif..stuv.",
                                    "..zA...fiiiiiiiiiiiiiiiiiiiif..qwxy.",
                                    ".BCD..ffiiiiiiiiiiiiiiiiiiiiff...zA.",
                                    "EFG....fiiiiiiiiiiiiiiiiiiiif...BCD.",
                                    "tfff...fiiiiiiiiiiiiiiiiiiiif..EFG..",
                                    ".......ffffffffffffffffffffff..tfff.",
                                    ".........f........f.......f.........",
                                    "....................................",
                                    ".......90a9......Hq......stuv.......",
                                    ".......bcde......rf......qwxy.......",
                                    ".......g/hg......If........zA.......",
                                    ".......ghhg.......f.......BCD.......",
                                    ".......jklb.......f......EFJ........",
                                    ".......mngo.......f......tfff.......",
                                    "...................................."};

/* XPM */
static const char *left_scl_xpm[] = {"36 36 72 1",
                                     " 	c None",
                                     ".	c #FFFFFF",
                                     "+	c #818181",
                                     "@	c #070707",
                                     "#	c #101010",
                                     "$	c #FAFAFA",
                                     "%	c #1D1D1D",
                                     "&	c #ACACAC",
                                     "*	c #151515",
                                     "=	c #121212",
                                     "-	c #929292",
                                     ";	c #242424",
                                     ">	c #D5D5D5",
                                     ",	c #C9C9C9",
                                     "'	c #2A2A2A",
                                     ")	c #5E5E5E",
                                     "!	c #000000",
                                     "~	c #C8C8C8",
                                     "{	c #D2D2D2",
                                     "]	c #090909",
                                     "^	c #060606",
                                     "/	c #FEFEFE",
                                     "(	c #FCFCFC",
                                     "_	c #CECECE",
                                     ":	c #444444",
                                     "<	c #E0E0E0",
                                     "[	c #464646",
                                     "}	c #E8E8E8",
                                     "|	c #282828",
                                     "1	c #D3D3D3",
                                     "2	c #D1D1D1",
                                     "3	c #A4A4A4",
                                     "4	c #484848",
                                     "5	c #F1F1F1",
                                     "6	c #7D7D7D",
                                     "7	c #050505",
                                     "8	c #7F7F7F",
                                     "9	c #F38C82",
                                     "0	c #E71E09",
                                     "a	c #E82612",
                                     "b	c #EA3826",
                                     "c	c #FBD8D5",
                                     "d	c #F9CEC9",
                                     "e	c #EA3E2C",
                                     "f	c #E71D08",
                                     "g	c #FEFCFC",
                                     "h	c #7D6F6F",
                                     "i	c #EA3C2A",
                                     "j	c #FAD7D3",
                                     "k	c #FAD5D1",
                                     "l	c #F2897E",
                                     "m	c #E71C07",
                                     "n	c #F28B80",
                                     "o	c #FEFAFA",
                                     "p	c #E9321F",
                                     "q	c #EF6D5F",
                                     "r	c #E71803",
                                     "s	c #F7B3AC",
                                     "t	c #E82B17",
                                     "u	c #E82814",
                                     "v	c #F49C93",
                                     "w	c #F9CDC8",
                                     "x	c #FAD6D2",
                                     "y	c #E7200B",
                                     "z	c #FAD2CE",
                                     "A	c #ED5546",
                                     "B	c #FCE2E0",
                                     "C	c #ED5748",
                                     "D	c #FCEAE8",
                                     "E	c #F6ACA5",
                                     "F	c #ED594A",
                                     "G	c #FDF2F1",
                                     ".......+@#+......$%......&*=-.......",
                                     ".......;>,'......)!......%~{].......",
                                     ".......^/(^.......!........_:.......",
                                     ".......^((^.......!.......<[}.......",
                                     ".......|12;.......!......345........",
                                     ".......67^8.......!......*!!!.......",
                                     "....................................",
                                     ".90a9...!........!.......!.....+@#+.",
                                     ".bcde..!!!!!!!!!!!!!!!!!!!!!!..;>,'.",
                                     ".f/gf..!hhhhhhhhhhhhhhhhhhhh!..^/(^.",
                                     ".fggf.!!hhhhhhhhhhhhhhhhhhhh!!.^((^.",
                                     ".ijkb..!hhhhhhhhhhhhhhhhhhhh!..|12;.",
                                     ".lmfn..!hhhhhhhhhhhhhhhhhhhh!..67^8.",
                                     ".......!hhhhhhhhhhhhhhhhhhhh!.......",
                                     ".......!hhhhhhhhhhhhhhhhhhhh!.......",
                                     "..op...!hhhhhhhhhhhhhhhhhhhh!...%...",
                                     "..qr...!hhhhhhhhhhhhhhhhhhhh!..)!...",
                                     "...r..!!hhhhhhhhhhhhhhhhhhhh!!..!...",
                                     "...r...!hhhhhhhhhhhhhhhhhhhh!...!...",
                                     "...r...!hhhhhhhhhhhhhhhhhhhh!...!...",
                                     "...r...!hhhhhhhhhhhhhhhhhhhh!...!...",
                                     ".......!hhhhhhhhhhhhhhhhhhhh!.......",
                                     ".stuv..!hhhhhhhhhhhhhhhhhhhh!..&*=-.",
                                     ".pwxy..!hhhhhhhhhhhhhhhhhhhh!..%~{].",
                                     "...zA.!!hhhhhhhhhhhhhhhhhhhh!!..._:.",
                                     "..BCD..!hhhhhhhhhhhhhhhhhhhh!...<[}.",
                                     ".EFG...!hhhhhhhhhhhhhhhhhhhh!..34...",
                                     ".trrr..!!!!!!!!!!!!!!!!!!!!!!..*!!!.",
                                     ".........!........!.......!.........",
                                     "....................................",
                                     ".......+@#+......$%......&*=-.......",
                                     ".......;>,'......)!......%~{].......",
                                     ".......^/(^.......!........_:.......",
                                     ".......^((^.......!.......<[}.......",
                                     ".......|12;.......!......345........",
                                     ".......67^8.......!......*!!!......."};

/* XPM */
static const char *right_scl_xpm[] = {"36 36 72 1",
                                      " 	c None",
                                      ".	c #FFFFFF",
                                      "+	c #818181",
                                      "@	c #070707",
                                      "#	c #101010",
                                      "$	c #FAFAFA",
                                      "%	c #1D1D1D",
                                      "&	c #ACACAC",
                                      "*	c #151515",
                                      "=	c #121212",
                                      "-	c #929292",
                                      ";	c #242424",
                                      ">	c #D5D5D5",
                                      ",	c #C9C9C9",
                                      "'	c #2A2A2A",
                                      ")	c #5E5E5E",
                                      "!	c #000000",
                                      "~	c #C8C8C8",
                                      "{	c #D2D2D2",
                                      "]	c #090909",
                                      "^	c #060606",
                                      "/	c #FEFEFE",
                                      "(	c #FCFCFC",
                                      "_	c #CECECE",
                                      ":	c #444444",
                                      "<	c #E0E0E0",
                                      "[	c #464646",
                                      "}	c #E8E8E8",
                                      "|	c #282828",
                                      "1	c #D3D3D3",
                                      "2	c #D1D1D1",
                                      "3	c #A4A4A4",
                                      "4	c #484848",
                                      "5	c #F1F1F1",
                                      "6	c #7D7D7D",
                                      "7	c #050505",
                                      "8	c #7F7F7F",
                                      "9	c #F38C82",
                                      "0	c #E71E09",
                                      "a	c #E82612",
                                      "b	c #EA3826",
                                      "c	c #FBD8D5",
                                      "d	c #F9CEC9",
                                      "e	c #EA3E2C",
                                      "f	c #7D6F6F",
                                      "g	c #E71D08",
                                      "h	c #FEFCFC",
                                      "i	c #EA3C2A",
                                      "j	c #FAD7D3",
                                      "k	c #FAD5D1",
                                      "l	c #F2897E",
                                      "m	c #E71C07",
                                      "n	c #F28B80",
                                      "o	c #FEFAFA",
                                      "p	c #E9321F",
                                      "q	c #EF6D5F",
                                      "r	c #E71803",
                                      "s	c #F7B3AC",
                                      "t	c #E82B17",
                                      "u	c #E82814",
                                      "v	c #F49C93",
                                      "w	c #F9CDC8",
                                      "x	c #FAD6D2",
                                      "y	c #E7200B",
                                      "z	c #FAD2CE",
                                      "A	c #ED5546",
                                      "B	c #FCE2E0",
                                      "C	c #ED5748",
                                      "D	c #FCEAE8",
                                      "E	c #F6ACA5",
                                      "F	c #ED594A",
                                      "G	c #FDF2F1",
                                      ".......+@#+......$%......&*=-.......",
                                      ".......;>,'......)!......%~{].......",
                                      ".......^/(^.......!........_:.......",
                                      ".......^((^.......!.......<[}.......",
                                      ".......|12;.......!......345........",
                                      ".......67^8.......!......*!!!.......",
                                      "....................................",
                                      ".+@#+...!........!.......!.....90a9.",
                                      ".;>,'..!!!!!!!!!!!!!!!!!!!!!!..bcde.",
                                      ".^/(^..!ffffffffffffffffffff!..g/hg.",
                                      ".^((^.!!ffffffffffffffffffff!!.ghhg.",
                                      ".|12;..!ffffffffffffffffffff!..ijkb.",
                                      ".67^8..!ffffffffffffffffffff!..lmgn.",
                                      ".......!ffffffffffffffffffff!.......",
                                      ".......!ffffffffffffffffffff!.......",
                                      ".$%....!ffffffffffffffffffff!...op..",
                                      ".)!....!ffffffffffffffffffff!...qr..",
                                      "..!...!!ffffffffffffffffffff!!...r..",
                                      "..!....!ffffffffffffffffffff!....r..",
                                      "..!....!ffffffffffffffffffff!....r..",
                                      "..!....!ffffffffffffffffffff!....r..",
                                      ".......!ffffffffffffffffffff!.......",
                                      ".&*=-..!ffffffffffffffffffff!..stuv.",
                                      ".%~{]..!ffffffffffffffffffff!..pwxy.",
                                      "..._:.!!ffffffffffffffffffff!!...zA.",
                                      "..<[}..!ffffffffffffffffffff!...BCD.",
                                      ".345...!ffffffffffffffffffff!..EFG..",
                                      ".*!!!..!!!!!!!!!!!!!!!!!!!!!!..trrr.",
                                      ".........!........!.......!.........",
                                      "....................................",
                                      ".......+@#+......$%......&*=-.......",
                                      ".......;>,'......)!......%~{].......",
                                      ".......^/(^.......!........_:.......",
                                      ".......^((^.......!.......<[}.......",
                                      ".......|12;.......!......345........",
                                      ".......67^8.......!......*!!!......."};

static const char *image2_data[] = {
    "74 77 171 2",
    "  	c None",
    ". 	c #FFFFFF",
    "+ 	c #E0E0E0",
    "@ 	c #FAFAFA",
    "# 	c #363636",
    "$ 	c #ABABAB",
    "% 	c #000000",
    "& 	c #BDBDBD",
    "* 	c #737373",
    "= 	c #858585",
    "- 	c #FFE8E8",
    "; 	c #FFF7F7",
    "> 	c #FFD1D1",
    ", 	c #FFF0F0",
    "' 	c #E6E6E6",
    ") 	c #FFB2B2",
    "! 	c #FFE6E6",
    "~ 	c #FF6666",
    "{ 	c #FFCCCC",
    "] 	c #C9C9C9",
    "^ 	c #939493",
    "/ 	c #808080",
    "( 	c #878787",
    "_ 	c #D6D6D6",
    ": 	c #969696",
    "< 	c #030303",
    "[ 	c #383838",
    "} 	c #4C4C4C",
    "| 	c #1A1A1A",
    "1 	c #080808",
    "2 	c #C2C2C2",
    "3 	c #EBEBEB",
    "4 	c #F0F0F0",
    "5 	c #8C8C8C",
    "6 	c #7A7A7A",
    "7 	c #A3A3A3",
    "8 	c #999999",
    "9 	c #A1A1A1",
    "0 	c #828282",
    "a 	c #949494",
    "b 	c #F2F2F2",
    "c 	c #474747",
    "d 	c #545454",
    "e 	c #0F0F0F",
    "f 	c #B5B5B5",
    "g 	c #CFCFCF",
    "h 	c #262626",
    "i 	c #333333",
    "j 	c #DEDEDE",
    "k 	c #FDFDFD",
    "l 	c #D7D7D7",
    "m 	c #B7B7B7",
    "n 	c #C4C4C4",
    "o 	c #F7F7F7",
    "p 	c #CCCCCC",
    "q 	c #2E2E2E",
    "r 	c #B2B2B2",
    "s 	c #DFDFDF",
    "t 	c #ACACAC",
    "u 	c #ADADAD",
    "v 	c #D3D3D3",
    "w 	c #CDCDCD",
    "x 	c #AAAAAA",
    "y 	c #D4C3C3",
    "z 	c #A9A9A9",
    "A 	c #AAA8A8",
    "B 	c #DB7F7F",
    "C 	c #E3E3E3",
    "D 	c #E5E5E5",
    "E 	c #F2DADA",
    "F 	c #B19999",
    "G 	c #FFC4C4",
    "H 	c #D2D2D2",
    "I 	c #FFD4D4",
    "J 	c #EA6565",
    "K 	c #E2E2E2",
    "L 	c #FFA1A1",
    "M 	c #C8C8C8",
    "N 	c #AFAFAF",
    "O 	c #FF9999",
    "P 	c #F5C6C6",
    "Q 	c #E9E9E9",
    "R 	c #B6B6B6",
    "S 	c #CFB4B4",
    "T 	c #B9B9B9",
    "U 	c #F5F5F5",
    "V 	c #ACA7A7",
    "W 	c #EAEAEA",
    "X 	c #F9F9F9",
    "Y 	c #B8B8B8",
    "Z 	c #C0A6A6",
    "` 	c #B4B4B4",
    " .	c #EFEFEF",
    "..	c #F2C3C3",
    "+.	c #EEEEEE",
    "@.	c #BCBCBC",
    "#.	c #C5C5C5",
    "$.	c #DADADA",
    "%.	c #BEBEBE",
    "&.	c #B0B0B0",
    "*.	c #FBFBFB",
    "=.	c #C6C6C6",
    "-.	c #B1B1B1",
    ";.	c #BABABA",
    ">.	c #AEAEAE",
    ",.	c #D0D0D0",
    "'.	c #FCFCFC",
    ").	c #141414",
    "!.	c #D8D8D8",
    "~.	c #C7C7C7",
    "{.	c #575757",
    "].	c #3D3D3D",
    "^.	c #C3C3C3",
    "/.	c #5E5E5E",
    "(.	c #7D7D7D",
    "_.	c #B3B3B3",
    ":.	c #F6C8C8",
    "<.	c #757575",
    "[.	c #919191",
    "}.	c #CAB3B3",
    "|.	c #EDEDED",
    "1.	c #ADA8A8",
    "2.	c #666666",
    "3.	c #2B2B2B",
    "4.	c #BBBBBB",
    "5.	c #C9AAAA",
    "6.	c #292929",
    "7.	c #D4D4D4",
    "8.	c #FBC9C9",
    "9.	c #E8E8E8",
    "0.	c #9C9C9C",
    "a.	c #E4E4E4",
    "b.	c #E7E7E7",
    "c.	c #CACACA",
    "d.	c #C1C1C1",
    "e.	c #E39090",
    "f.	c #CBCBCB",
    "g.	c #ADA3A3",
    "h.	c #C8B0B0",
    "i.	c #F59A9A",
    "j.	c #C0A9A9",
    "k.	c #C9BFBF",
    "l.	c #7A4747",
    "m.	c #A18C8C",
    "n.	c #8C8282",
    "o.	c #332929",
    "p.	c #242424",
    "q.	c #0A0A0A",
    "r.	c #454545",
    "s.	c #595959",
    "t.	c #525252",
    "u.	c #5C5C5C",
    "v.	c #D9D9D9",
    "w.	c #121212",
    "x.	c #8A8A8A",
    "y.	c #171717",
    "z.	c #A8A8A8",
    "A.	c #696969",
    "B.	c #707070",
    "C.	c #9E9E9E",
    "D.	c #404040",
    "E.	c #616161",
    "F.	c #8F8F8F",
    "G.	c #4F4F4F",
    "H.	c #050505",
    "I.	c #DBDBDB",
    "J.	c #4A4A4A",
    "K.	c #212121",
    "L.	c #3B3B3B",
    "M.	c #636363",
    "N.	c #D1D1D1",
    ". . . . . . . . . . + . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . @ # . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . $ % & . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . * % = . . . . . . . . . . . - . . . . . . . . . . . ; > "
    ". . . . . . . . . . . . , . . . . . . . . . . . - - . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . ] ^ / ( _ . ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . : < [ } | 1 ] ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . 2 ' . . 3 % / ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . . 4 % 5 ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . . 6 1 + ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . 7 < 8 @ 5 % 9 . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . 0 < a . b c % d . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . b d e f . . . ' % . . . . . . . . . . . . ) . . . . . . . . . . . ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". g h i j . . . . ' % . . . . . . . . . . . . ) . . . . k l m m m n o ! ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    "p 1 q $ r r r g . ' % . . . . . . . . . . . . ) . . . s t u ] v w r x y ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    "7 i i i i i i : . ' % . . . . . . . . . . . . ) . . p z _ . . . . . l A B "
    ". . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . ) . C x D . . . . . . . E F "
    "] . . . . . . . . . . . { . . . . . . . . . . . ) ) . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . G . u H . . . . . . . . I J "
    "x K . . . . . . . . . . { . . . . . . . . . . . L G . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { M N . . . . . . . . . { ~ "
    "_ $ o . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . P x Q . . . . . . . . . { ~ "
    ". & R . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . S T . . . . . . . . . . { ~ "
    ". U $ _ . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . 4 V W . . . . . . . . . . { ~ "
    ". . K x X . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . Y Z . . . . . . . . . . . { ~ "
    ". . . ` n . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . .  .x ... . . . . . . . . . . { ~ "
    ". . . +.$ +.. . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . @.n { . . . . . . . . . . . { ~ "
    ". . . . #.R . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . x +.{ . . . . . . . . . . . { ~ "
    ". . . . X z o . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . $.u . { . . . . . . . . . . . { ~ "
    ". . . . . %.&.*.. . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . ` =.. { . . . . . . . . . . . { ~ "
    ". . . . . . -.=.. . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . 4 z X . { . . . . . . . . . . . { ~ "
    ". . . . . . + t . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . #.;.. . { . . . . . . . . . . . { ~ "
    ". . . . . . . >.v . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . u $.. . { . . . . . . . . . . . { ~ "
    ". . . . . . . _ t . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . n 2 . ' % . . . . . . . + t k . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . t ,.. . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . '.& ).5 . ' % . . . . . . . m #.. . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . !.t . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . f % % % ~.. ' % . . . . . . . x W . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . N ] . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . a 1 @ . ' % . . . . . . s u . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . l u k { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . {.].. . ' % . . . . . . %.%.. . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . r ^.{ . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . | 6 . U /.% (.. . . . . _.' . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . 4 u :.. . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . + % r . o <.% [.. . . . o $ '.. . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . ^.}.. . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . 7 % |.. . ' % . . . . . ,._.. . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . o 1.D . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . 2.3.. . . ' % . . . . . 4.=.. . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . 5.-.. . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . p p 6.} p + . ' % . . . . . _.7.. . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . 8.t ,.. . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". 9.i i i i i 0.. ' % . . . . . x +.. . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { C x a.. . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . x . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . ,.u . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . b.t . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . &.4.. . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . ^.T . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . X u c.. . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . m H . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . +.$ p . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . t C . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . +.x _ . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . z X . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . + x _ . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . U x . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . K x H . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . a.f . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . ' &.d.X O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . $.-.. . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . X _.>.e.{ . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . M 4.. . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . f.g.h.U . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . $.,.. . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . i.j.x Y b . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { b.@.x &.~.D . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . U f.m @.. . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . { ~ "
    ". . . . . . . . . . . . { . . . . . . . . . . . O { . . . . . . . . . . "
    ". ",
    ". . . . . . . . . ' % . . . . . . . . . . . . { . . . . . . . . . . . "
    "k.l.. . . . . . . . . . . . { . . . . . . . . . . . m.n.. . . . . . . 2 C "
    ". . ",
    ". . . . . . . . . ' % i i i i i i i i i i i i o.i i i i i i i i i i i "
    "p.q.i i i i i i i i i i i i o.i i i i i i i i i i i | ).i i i i i i i ).< "
    "r.~.",
    ". . . . . . . . . . Y r r r r r r r r r r r r r r r r r r r r r r r r "
    "(.p.r r r r r r r r r r r r r r r r r r r r r r r r s.c r r r r r r r "
    "t.u.2 . ",
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 4 ] "
    ". . . . . . . . . . . . . . . . . . . . . . . . + v.. . . . . . . . . . "
    ". ",
    ". . . . '.= ).% w.[.. . . . . . . . . . . . . . . . . . . . . . . . . . "
    "x.8 . . . . . . . . . . . . . . . . . . . . . ~.2.[ i c 2 . . . . . . . . "
    ". ",
    ". . . . 2.y.z.p A.< j . . . . . . . . . . . . . . . . . . . . . . b &.B.< "
    "C.. . . . . . . . . . . . . . . . . . . . . D.t.[.a 3.1 9.. . . . . . . "
    ". ",
    ". . . p % &.. . + % r . . . . . . . . . . . . . . . . . . . . . . 2 } i % "
    "v.. . . . . . . . . . . . . . . . . . . . . 3 . . . &.% p . . . . . . . "
    ". ",
    ". . . E.h . . . ' % r . . . . . . . . . . . . . . . . . . . . . . . . / "
    ").. . . . . . . . . . . . . . . . . . . . . . . . . . F.< 9.. . . . . . . "
    ". ",
    ". . . | * . . . ~.% ~.. . . . . . . . . . . . . . . . . . . . . . . . c "
    "G.. . . . . . . . . . . . . . . . . . . . . . . . . + ).E.. . . . . . . . "
    ". ",
    ". . j % &.. . . a H.U . . . . . . . . . . . . . . . . . . . . . . . "
    "'.q.x.. . . . . . . . . . . . . . . . . . . . . . . . j h # U . . . . . . "
    ". . . ",
    ". . Y % I.. . . G.D.. . . . . . . . . . . . . . . . . . . . . . . . p % "
    "~.. . . . . . . . . . . . . . . . . . . . . . . n y.J.4 . . . . . . . . . "
    ". ",
    ". . r % ' . . 9.1 8 . . . . . . . . . . . . . . . . . . . . . . . . a 1 o "
    ". . . . . . . . . . . . . . . . . . . . . . [.H.<.'.. . . . . . . . . . "
    ". ",
    ". . & % 2 . '.s.K.U . . . . . . . . . . . . . . . . . . . . . . . . d L.. "
    ". . . . . . . . . . . . . . . . . . . . |.t.w.r . . . . . . . . . . . . "
    ". ",
    ". . @ 6.).} h y.p . . . . . . . . . . . . . . . . . . . . . . @ / / ).# / "
    "Y . . . . . . . . . . . . . . . . . . . M.% J.2.2.2.2.N.. . . . . . . . "
    ". ",
    ". . . 3 F./ a 4 . . . . . . . . . . . . . . . . . . . . . . . |./ / / / / "
    "n . . . . . . . . . . . . . . . . . . . 0./ / / / / / 9.. . . . . . . . "
    ". "};

static const char *image3_data[] = {
    "74 77 168 2",
    "  	c None",
    ". 	c #FFFFFF",
    "+ 	c #E0E0E0",
    "@ 	c #FAFAFA",
    "# 	c #363636",
    "$ 	c #ABABAB",
    "% 	c #000000",
    "& 	c #BDBDBD",
    "* 	c #737373",
    "= 	c #858585",
    "- 	c #E6E6E6",
    "; 	c #C9C9C9",
    "> 	c #939493",
    ", 	c #808080",
    "' 	c #878787",
    ") 	c #D6D6D6",
    "! 	c #969696",
    "~ 	c #030303",
    "{ 	c #383838",
    "] 	c #4C4C4C",
    "^ 	c #1A1A1A",
    "/ 	c #080808",
    "( 	c #C2C2C2",
    "_ 	c #EBEBEB",
    ": 	c #F0F0F0",
    "< 	c #8C8C8C",
    "[ 	c #7A7A7A",
    "} 	c #A3A3A3",
    "| 	c #999999",
    "1 	c #A1A1A1",
    "2 	c #FFE6E6",
    "3 	c #FFF0F0",
    "4 	c #828282",
    "5 	c #949494",
    "6 	c #F2F2F2",
    "7 	c #474747",
    "8 	c #545454",
    "9 	c #FF8080",
    "0 	c #FFB2B2",
    "a 	c #0F0F0F",
    "b 	c #B5B5B5",
    "c 	c #CFCFCF",
    "d 	c #262626",
    "e 	c #333333",
    "f 	c #DEDEDE",
    "g 	c #FDFDFD",
    "h 	c #D7D7D7",
    "i 	c #B7B7B7",
    "j 	c #C4C4C4",
    "k 	c #F7F7F7",
    "l 	c #CCCCCC",
    "m 	c #2E2E2E",
    "n 	c #B2B2B2",
    "o 	c #DFDFDF",
    "p 	c #ACACAC",
    "q 	c #ADADAD",
    "r 	c #D3D3D3",
    "s 	c #CDCDCD",
    "t 	c #AAAAAA",
    "u 	c #D4D4D4",
    "v 	c #A9A9A9",
    "w 	c #DBDBDB",
    "x 	c #E3E3E3",
    "y 	c #E5E5E5",
    "z 	c #B1B1B1",
    "A 	c #D2D2D2",
    "B 	c #EAEAEA",
    "C 	c #E2E2E2",
    "D 	c #C8C8C8",
    "E 	c #AFAFAF",
    "F 	c #F5F5F5",
    "G 	c #E9E9E9",
    "H 	c #B6B6B6",
    "I 	c #B9B9B9",
    "J 	c #F9F9F9",
    "K 	c #B8B8B8",
    "L 	c #C0C0C0",
    "M 	c #B4B4B4",
    "N 	c #EFD9D9",
    "O 	c #AAA8A8",
    "P 	c #F2DBDB",
    "Q 	c #EED6D6",
    "R 	c #ABA8A8",
    "S 	c #EED8D8",
    "T 	c #BCB1B1",
    "U 	c #C4B6B6",
    "V 	c #C5B7B7",
    "W 	c #B6ADAD",
    "X 	c #EEEEEE",
    "Y 	c #DADADA",
    "Z 	c #BEBEBE",
    "` 	c #B0B0B0",
    " .	c #FBFBFB",
    "..	c #C6C6C6",
    "+.	c #C5C5C5",
    "@.	c #BABABA",
    "#.	c #AEAEAE",
    "$.	c #D0D0D0",
    "%.	c #FCFCFC",
    "&.	c #141414",
    "*.	c #D8D8D8",
    "=.	c #C7C7C7",
    "-.	c #575757",
    ";.	c #3D3D3D",
    ">.	c #C3C3C3",
    ",.	c #5E5E5E",
    "'.	c #7D6E6E",
    ").	c #FF9999",
    "!.	c #B39D9D",
    "~.	c #E69292",
    "{.	c #F09393",
    "].	c #ADA2A2",
    "^.	c #F49999",
    "/.	c #FFC2C2",
    "(.	c #757575",
    "_.	c #918787",
    ":.	c #F7AEAE",
    "<.	c #ABA6A6",
    "[.	c #FCB2B2",
    "}.	c #C3A0A0",
    "|.	c #C7A4A4",
    "1.	c #FFD1D1",
    "2.	c #EDEDED",
    "3.	c #B3B3B3",
    "4.	c #666666",
    "5.	c #2B2B2B",
    "6.	c #BBBBBB",
    "7.	c #292929",
    "8.	c #E8E8E8",
    "9.	c #9C9C9C",
    "0.	c #E4E4E4",
    "a.	c #E7E7E7",
    "b.	c #CACACA",
    "c.	c #E4A7A7",
    "d.	c #B59F9F",
    "e.	c #E6A7A7",
    "f.	c #B0A3A3",
    "g.	c #C19E9E",
    "h.	c #F9B0B0",
    "i.	c #CBCBCB",
    "j.	c #F4F4F4",
    "k.	c #BCBCBC",
    "l.	c #707070",
    "m.	c #242424",
    "n.	c #0A0A0A",
    "o.	c #454545",
    "p.	c #7D7D7D",
    "q.	c #595959",
    "r.	c #525252",
    "s.	c #5C5C5C",
    "t.	c #D9D9D9",
    "u.	c #121212",
    "v.	c #919191",
    "w.	c #8A8A8A",
    "x.	c #171717",
    "y.	c #A8A8A8",
    "z.	c #696969",
    "A.	c #9E9E9E",
    "B.	c #404040",
    "C.	c #616161",
    "D.	c #8F8F8F",
    "E.	c #4F4F4F",
    "F.	c #050505",
    "G.	c #4A4A4A",
    "H.	c #212121",
    "I.	c #3B3B3B",
    "J.	c #636363",
    "K.	c #D1D1D1",
    ". . . . . . . . . . + . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . ",
    ". . . . . . . . . @ # . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . $ % & . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . * % = . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . ; > , ' ) . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . ! ~ { ] ^ / ; - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . ( - . . _ % , - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . : % < - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . [ / + - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . } ~ | @ < % 1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 "
    "2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 . . "
    ". ",
    ". . . . 4 ~ 5 . 6 7 % 8 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 "
    "9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 0 . . "
    ". ",
    ". . 6 8 a b . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". c d e f . . . . - % . . . . . . . . . . . . . . . . . g h i i i j k . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    "l / m $ n n n c . - % . . . . . . . . . . . . . . . . o p q ; r s n t u . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    "} e e e e e e ! . - % . . . . . . . . . . . . . . . l v ) . . . . . h t w "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . x t y . . . . . . . 6 z "
    "; . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . q A . . . . . . . . . B "
    "t C . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . D E . . . . . . . . . . . "
    ") $ k . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . F t G . . . . . . . . . . . "
    ". & H . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . c I . . . . . . . . . . . . "
    ". F $ ) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . : p B . . . . . . . . . . . . "
    ". . C t J . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . K L . . . . . . . . . . . . . "
    ". . . M j . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % 2 2 2 2 2 2 2 2 2 2 N O P 2 2 2 2 2 2 2 2 2 2 2 2 2 "
    "2 2 2 Q R S 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 . . "
    ". ",
    ". . . . . . . . . - % 2 2 2 2 2 2 2 2 2 2 T U 2 2 2 2 2 2 2 2 2 2 2 2 2 2 "
    "2 2 2 2 V W 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . t X . . . . . . . . . . . . . . "
    ". . . . J v k . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . Y q . . . . . . . . . . . . . . . "
    ". . . . . Z `  .. . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . M ... . . . . . . . . . . . . . . "
    ". . . . . . z ... . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . : v J . . . . . . . . . . . . . . . "
    ". . . . . . + p . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . +.@.. . . . . . . . . . . . . . . . "
    ". . . . . . . #.r . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . q Y . . . . . . . . . . . . . . . . "
    ". . . . . . . ) p . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . j ( . - % . . . . . . . + p g . . . . . . . . . . . . . . . . "
    ". . . . . . . . p $.. . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . %.& &.< . - % . . . . . . . i +.. . . . . . . . . . . . . . . . . "
    ". . . . . . . . *.p . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . b % % % =.. - % . . . . . . . t B . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . E ; . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . 5 / @ . - % . . . . . . o q . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . h q g . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . -.;.. . - % . . . . . . Z Z . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . n >.. . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . ^ [ . F ,.% "
    "'.).).).).).!.~.).).).).).).).).).).).).).).).).).).).).).).).).).).).).{."
    "].^.).).).).).).).).).).).).).).).).).).).)./.. . . ",
    ". . . . + % n . k (.% _.0 0 0 0 :.<.[.0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
    "0 0 0 0 0 0 0 0 0 0 0 }.|.0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1.. . "
    ". ",
    ". . . . } % 2.. . - % . . . . . $.3.. . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . k p y . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . 4.5.. . . - % . . . . . 6.... . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . ; z . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . l l 7.] l + . - % . . . . . 3.u . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . .  .p $.. . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". 8.e e e e e 9.. - % . . . . . t X . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . x t 0.. . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . t . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . $.q . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . a.p . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . ` 6.. . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . >.I . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . J q b.. . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . i A . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . X $ l . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . p x . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . X t ) . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . v J . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . + t ) . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . F t . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . C t A . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % 0 0 0 c.d.0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
    "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 e.f.g.h.0 0 0 0 0 0 0 0 0 1.. . "
    ". ",
    ". . . . . . . . . - % . . . Y z . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . J 3.#.x . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . D 6.. . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . i.p D F . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . Y $.. . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . j.L t K 6 . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . a.k.t ` =.y . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . F i.i k.. . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    ". ",
    ". . . . . . . . . - % . . . . . . . . . . . . . . . . . . . . . . . . ; "
    "l.. . . . . . . . . . . . . . . . . . . . . . . . 1 < . . . . . . . ( x . "
    ". ",
    ". . . . . . . . . - % e e e e e e e e e e e e e e e e e e e e e e e e "
    "m.n.e e e e e e e e e e e e e e e e e e e e e e e e ^ &.e e e e e e e &.~ "
    "o.=.",
    ". . . . . . . . . . K n n n n n n n n n n n n n n n n n n n n n n n n "
    "p.m.n n n n n n n n n n n n n n n n n n n n n n n n q.7 n n n n n n n "
    "r.s.( . ",
    ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . : ; "
    ". . . . . . . . . . . . . . . . . . . . . . . . + t.. . . . . . . . . . "
    ". ",
    ". . . . %.= &.% u.v.. . . . . . . . . . . . . . . . . . . . . . . . . . "
    "w.| . . . . . . . . . . . . . . . . . . . . . =.4.{ e 7 ( . . . . . . . . "
    ". ",
    ". . . . 4.x.y.l z.~ f . . . . . . . . . . . . . . . . . . . . . . 6 ` l.~ "
    "A.. . . . . . . . . . . . . . . . . . . . . B.r.v.5 5./ 8.. . . . . . . "
    ". ",
    ". . . l % ` . . + % n . . . . . . . . . . . . . . . . . . . . . . ( ] e % "
    "t.. . . . . . . . . . . . . . . . . . . . . _ . . . ` % l . . . . . . . "
    ". ",
    ". . . C.d . . . - % n . . . . . . . . . . . . . . . . . . . . . . . . , "
    "&.. . . . . . . . . . . . . . . . . . . . . . . . . . D.~ 8.. . . . . . . "
    ". ",
    ". . . ^ * . . . =.% =.. . . . . . . . . . . . . . . . . . . . . . . . 7 "
    "E.. . . . . . . . . . . . . . . . . . . . . . . . . + &.C.. . . . . . . . "
    ". ",
    ". . f % ` . . . 5 F.F . . . . . . . . . . . . . . . . . . . . . . . "
    "%.n.w.. . . . . . . . . . . . . . . . . . . . . . . . f d # F . . . . . . "
    ". . . ",
    ". . K % w . . . E.B.. . . . . . . . . . . . . . . . . . . . . . . . l % "
    "=.. . . . . . . . . . . . . . . . . . . . . . . j x.G.: . . . . . . . . . "
    ". ",
    ". . n % - . . 8./ | . . . . . . . . . . . . . . . . . . . . . . . . 5 / k "
    ". . . . . . . . . . . . . . . . . . . . . . v.F.(.%.. . . . . . . . . . "
    ". ",
    ". . & % ( . %.q.H.F . . . . . . . . . . . . . . . . . . . . . . . . 8 I.. "
    ". . . . . . . . . . . . . . . . . . . . 2.r.u.n . . . . . . . . . . . . "
    ". ",
    ". . @ 7.&.] d x.l . . . . . . . . . . . . . . . . . . . . . . @ , , &.# , "
    "K . . . . . . . . . . . . . . . . . . . J.% G.4.4.4.4.K.. . . . . . . . "
    ". ",
    ". . . _ D., 5 : . . . . . . . . . . . . . . . . . . . . . . . 2., , , , , "
    "j . . . . . . . . . . . . . . . . . . . 9., , , , , , 8.. . . . . . . . "
    ". "};

static const char *image4_data[] = {"35 32 4 1",
                                    "# c #000000",
                                    "a c #bfbfbf",
                                    "b c #ff0000",
                                    ". c #ffffff",
                                    "...................................",
                                    ".........#.....#.....#.....#.......",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.......",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.......",
                                    "....##########################.....",
                                    "....#aaaaaaaaaaaaaaaaaaaaaaaa#.....",
                                    "..###aaaaaaaaaaaaaaaaaaaaaaaa###...",
                                    "....#aaaaaaaaaaaaaaaaaaaaaaaa#.....",
                                    "..###aaaaaaaaaaaaaaaaaaaaaaaa###...",
                                    "....#aaaaaaaaaaaaaaaaaaaaaaaa#.....",
                                    ".####aaaaaaaaaaaaaaaa#aaaaaaa####..",
                                    "....#aaaaaaaaaaaaaaa#a#aaaaaa#.....",
                                    "..###aaaaaaaaaaaaaaa#a#aaaaaa###...",
                                    "....#aaaaaaaaaaaaaa#aaa#aaaaa#.....",
                                    "..###aaaaaaa#aaaaaa#aaa#aaaaa###...",
                                    "....#aaaaaa#a#aaaa#aaaaa#aaaa#.....",
                                    ".####aaaaaa#a#aaaa#aaaaa#aaaa####..",
                                    "....#aaaaa#aaa#aa#aaaaaaa#aaa#.....",
                                    "..###aaaaa#aaa###aaaaaaaaa######...",
                                    "....#aaaa#aaaaa#aaaaaaaaaaaaa#.....",
                                    "..###aaaa#aaaaa#aaaaaaaaaaaaa###...",
                                    "....#aaa#aaaaaaa#aaaaaaaaaaaa#.....",
                                    ".#######aaaaaaaaa#####aaaaaaa####..",
                                    "....#aaaaaaaaaaaaaaaaaaaaaaaa#.....",
                                    "..###aaaaaaaaaaaaaaaaaaaaaaaa###...",
                                    "....#aaaaaaaaaaaaaaaaaaaaaaaa#.....",
                                    "....bbbbbbbbbbbbbbbbbbbbbbbbbb.....",
                                    "....bbbbbbbbbbbbbbbbbbbbbbbbbb.....",
                                    ".....b.b.b.b.b.b.b.b.b.b.b.b.......",
                                    ".....b.b.b.b.b.b.b.b.b.b.b.b.......",
                                    ".........b.....b.....b.....b.......",
                                    "..................................."};

static const char *image5_data[] = {"33 32 4 1",
                                    "# c #000000",
                                    "b c #bfbfbf",
                                    "a c #ff0000",
                                    ". c #ffffff",
                                    ".................................",
                                    ".........#.....#.....#.....#.....",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    "....aa########################...",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    "..aaaabbbbbbbbbbbbbbbbbbbbbbb###.",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    "..aaaabbbbbbbbbbbbbbbbbbbbbbb###.",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    ".aaaaabbbbbbbbbbbbbbb#bbbbbbb####",
                                    "....aabbbbbbbbbbbbbb#b#bbbbbb#...",
                                    "..aaaabbbbbbbbbbbbbb#b#bbbbbb###.",
                                    "....aabbbbbbbbbbbbb#bbb#bbbbb#...",
                                    "..aaaabbbbbb#bbbbbb#bbb#bbbbb###.",
                                    "....aabbbbb#b#bbbb#bbbbb#bbbb#...",
                                    ".aaaaabbbbb#b#bbbb#bbbbb#bbbb####",
                                    "....aabbbb#bbb#bb#bbbbbbb#bbb#...",
                                    "..aaaabbbb#bbb###bbbbbbbbb######.",
                                    "....aabbb#bbbbb#bbbbbbbbbbbbb#...",
                                    "..aaaabbb#bbbbb#bbbbbbbbbbbbb###.",
                                    "....aabb#bbbbbbb#bbbbbbbbbbbb#...",
                                    ".aaaaa##bbbbbbbbb#####bbbbbbb####",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    "..aaaabbbbbbbbbbbbbbbbbbbbbbb###.",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    "....aabbbbbbbbbbbbbbbbbbbbbbb#...",
                                    "....aa########################...",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    ".........#.....#.....#.....#.....",
                                    "................................."};

static const char *image6_data[] = {"34 34 4 1",
                                    "a c #000000",
                                    "b c #bfbfbf",
                                    "# c #ff0000",
                                    ". c #ffffff",
                                    "..................................",
                                    "..................................",
                                    ".........#.....#.....#.....#......",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#......",
                                    ".....#.#.#.#.#.#.#.#.#.#.#.#......",
                                    "....##########################....",
                                    "....##########################....",
                                    "..aaabbbbbbbbbbbbbbbbbbbbbbbbaaa..",
                                    "....abbbbbbbbbbbbbbbbbbbbbbbba....",
                                    "..aaabbbbbbbbbbbbbbbbbbbbbbbbaaa..",
                                    "....abbbbbbbbbbbbbbbbbbbbbbbba....",
                                    ".aaaabbbbbbbbbbbbbbbbabbbbbbbaaaa.",
                                    "....abbbbbbbbbbbbbbbababbbbbba....",
                                    "..aaabbbbbbbbbbbbbbbababbbbbbaaa..",
                                    "....abbbbbbbbbbbbbbabbbabbbbba....",
                                    "..aaabbbbbbbabbbbbbabbbabbbbbaaa..",
                                    "....abbbbbbababbbbabbbbbabbbba....",
                                    ".aaaabbbbbbababbbbabbbbbabbbbaaaa.",
                                    "....abbbbbabbbabbabbbbbbbabbba....",
                                    "..aaabbbbbabbbaaabbbbbbbbbaaaaaa..",
                                    "....abbbbabbbbbabbbbbbbbbbbbba....",
                                    "..aaabbbbabbbbbabbbbbbbbbbbbbaaa..",
                                    "....abbbabbbbbbbabbbbbbbbbbbba....",
                                    ".aaaaaaabbbbbbbbbaaaaabbbbbbbaaaa.",
                                    "....abbbbbbbbbbbbbbbbbbbbbbbba....",
                                    "..aaabbbbbbbbbbbbbbbbbbbbbbbbaaa..",
                                    "....abbbbbbbbbbbbbbbbbbbbbbbba....",
                                    "....abbbbbbbbbbbbbbbbbbbbbbbba....",
                                    "....aaaaaaaaaaaaaaaaaaaaaaaaaa....",
                                    ".....a.a.a.a.a.a.a.a.a.a.a.a......",
                                    ".....a.a.a.a.a.a.a.a.a.a.a.a......",
                                    ".........a.....a.....a.....a......",
                                    "..................................",
                                    ".................................."};

static const char *image7_data[] = {"32 32 4 1",
                                    "# c #000000",
                                    "b c #bfbfbf",
                                    "a c #ff0000",
                                    ". c #ffffff",
                                    "................................",
                                    "........#.....#.....#.....#.....",
                                    "....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    "....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    "...########################aa...",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    ".###bbbbbbbbbbbbbbbbbbbbbbbaaaa.",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    ".###bbbbbbbbbbbbbbbbbbbbbbbaaaa.",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    "####bbbbbbbbbbbbbbbb#bbbbbbaaaaa",
                                    "...#bbbbbbbbbbbbbbb#b#bbbbbaa...",
                                    ".###bbbbbbbbbbbbbbb#b#bbbbbaaaa.",
                                    "...#bbbbbbbbbbbbbb#bbb#bbbbaa...",
                                    ".###bbbbbbb#bbbbbb#bbb#bbbbaaaa.",
                                    "...#bbbbbb#b#bbbb#bbbbb#bbbaa...",
                                    "####bbbbbb#b#bbbb#bbbbb#bbbaaaaa",
                                    "...#bbbbb#bbb#bb#bbbbbbb#bbaa...",
                                    ".###bbbbb#bbb###bbbbbbbbb##aaaa.",
                                    "...#bbbb#bbbbb#bbbbbbbbbbbbaa...",
                                    ".###bbbb#bbbbb#bbbbbbbbbbbbaaaa.",
                                    "...#bbb#bbbbbbb#bbbbbbbbbbbaa...",
                                    "#######bbbbbbbbb#####bbbbbbaaaaa",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    ".###bbbbbbbbbbbbbbbbbbbbbbbaaaa.",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    "...#bbbbbbbbbbbbbbbbbbbbbbbaa...",
                                    "...########################aa...",
                                    "....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    "....#.#.#.#.#.#.#.#.#.#.#.#.....",
                                    "........#.....#.....#.....#.....",
                                    "................................"};

namespace {
Mantid::Kernel::Logger g_log("AxisDialog");
}

///////////////////
// Public Functions
///////////////////

/** The constructor for a single set of widgets containing parameters for the
 * scale of an axis.
 *  @param app :: the containing application window
 *  @param g :: the graph the dialog is settign the options for
 *  @param fl :: The QT flags for this window
 */
AxesDialog::AxesDialog(ApplicationWindow *app, Graph *g, Qt::WFlags fl)
    : QDialog(g, fl), m_app(app), m_graph(g) {
  QPixmap image4((const char **)image4_data);
  QPixmap image5((const char **)image5_data);
  QPixmap image6((const char **)image6_data);
  QPixmap image7((const char **)image7_data);
  setWindowTitle(tr("MantidPlot - General Plot Options"));

  m_generalDialog = new QTabWidget();
  m_generalModified = false;

  initScalesPage();
  initAxesPage();
  initGridPage();
  initGeneralPage();

  // Connect scale details to axis details in order to disable scale options
  // when an axis is not shown
  auto scaleIter = m_Scale_list.begin();
  auto axisIter = m_Axis_list.begin();
  while ((scaleIter != m_Scale_list.end()) && (axisIter != m_Axis_list.end())) {
    connect(*axisIter, SIGNAL(axisShowChanged(bool)), *scaleIter,
            SLOT(axisEnabled(bool)));

    ++scaleIter;
    ++axisIter;
  }

  QHBoxLayout *bottomButtons = new QHBoxLayout();
  bottomButtons->addStretch();

  m_btnApply = new QPushButton();
  m_btnApply->setText(tr("&Apply"));
  bottomButtons->addWidget(m_btnApply);

  m_btnOk = new QPushButton();
  m_btnOk->setText(tr("&OK"));
  m_btnOk->setDefault(true);
  bottomButtons->addWidget(m_btnOk);

  m_btnCancel = new QPushButton();
  m_btnCancel->setText(tr("&Cancel"));
  bottomButtons->addWidget(m_btnCancel);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_generalDialog);
  mainLayout->addLayout(bottomButtons);

  m_lastPage = m_scalesPage;
  connect(m_btnOk, SIGNAL(clicked()), this, SLOT(accept()));
  connect(m_btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(m_btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(m_generalDialog, SIGNAL(currentChanged(QWidget *)), this,
          SLOT(pageChanged(QWidget *)));
}

AxesDialog::~AxesDialog() {}

/**Applies changes then closes the dialog
 *
 */
void AxesDialog::accept() {
  m_btnOk->setFocus();
  if (pressToGraph())
    close();
}
/** Applies the changes to the graph without closing the window
 *
 */
void AxesDialog::apply() {
  m_btnApply->setFocus();
  pressToGraph();
}
/**shows the Axes tab
 *
 */
void AxesDialog::showAxesPage() {
  if (m_generalDialog->currentWidget() != dynamic_cast<QWidget *>(m_axesPage)) {
    m_generalDialog->setCurrentWidget(m_axesPage);
  }
}

/**shows the Grid tab
 *
 */
void AxesDialog::showGridPage() {
  if (m_generalDialog->currentWidget() != dynamic_cast<QWidget *>(m_gridPage)) {
    m_generalDialog->setCurrentWidget(m_gridPage);
  }
}

/**shows the General tab
 *
 */
void AxesDialog::showGeneralPage() {
  const int generalIndex = m_generalDialog->indexOf(m_generalPage);
  m_generalDialog->setCurrentIndex(generalIndex);
}

/**launches the dialog
 *
 */
int AxesDialog::exec() {
  m_lstScales->setCurrentRow(0);
  m_lstGrid->setCurrentRow(0);
  m_lstAxes->setCurrentRow(0);
  setModal(true);
  show();
  return 0;
}

/**sets the current shown axis of scale and axis tabs
 *
 */
void AxesDialog::setCurrentScale(int axisPos) {
  int axis = -1;
  switch (axisPos) {
  case QwtScaleDraw::LeftScale: {
    axis = 1;
    break;
  }
  case QwtScaleDraw::BottomScale: {
    axis = 0;
    break;
  }
  case QwtScaleDraw::RightScale: {
    axis = 3;
    break;
  }
  case QwtScaleDraw::TopScale: {
    axis = 2;
    break;
  }
  }
  if (m_generalDialog->currentWidget() ==
      dynamic_cast<QWidget *>(m_scalesPage)) {
    m_lstScales->setCurrentRow(axis);
  } else if (m_generalDialog->currentWidget() ==
             dynamic_cast<QWidget *>(m_axesPage)) {
    m_lstAxes->setCurrentRow(axis);
  }
}

///////////////////
// Private functions
///////////////////

/**initialises the scales tab
 *
 */
void AxesDialog::initScalesPage() {
  m_scalesPage = new QWidget();
  scalesLayout = new QHBoxLayout(m_scalesPage);

  QPixmap image0((const char **)bottom_scl_xpm);
  QPixmap image1((const char **)left_scl_xpm);
  QPixmap image2((const char **)top_scl_xpm);
  QPixmap image3((const char **)right_scl_xpm);

  m_lstScales = new QListWidget();
  m_scalePrefsArea = new QStackedLayout();
  scalesLayout->addWidget(m_lstScales);
  scalesLayout->addLayout(m_scalePrefsArea);

  QListWidgetItem *listBottom = new QListWidgetItem(image0, tr("Bottom"));
  QListWidgetItem *listLeft = new QListWidgetItem(image1, tr("Left"));
  QListWidgetItem *listTop = new QListWidgetItem(image2, tr("Top"));
  QListWidgetItem *listRight = new QListWidgetItem(image3, tr("Right"));

  ScaleDetails *prefsBottom =
      new ScaleDetails(m_app, m_graph, QwtPlot::xBottom);
  ScaleDetails *prefsLeft = new ScaleDetails(m_app, m_graph, QwtPlot::yLeft);
  ScaleDetails *prefsTop = new ScaleDetails(m_app, m_graph, QwtPlot::xTop);
  ScaleDetails *prefsRight = new ScaleDetails(m_app, m_graph, QwtPlot::yRight);

  m_scalePrefsArea->addWidget(prefsBottom);
  m_scalePrefsArea->addWidget(prefsLeft);
  m_scalePrefsArea->addWidget(prefsTop);
  m_scalePrefsArea->addWidget(prefsRight);

  m_lstScales->addItem(listBottom);
  m_lstScales->addItem(listLeft);
  m_lstScales->addItem(listTop);
  m_lstScales->addItem(listRight);

  m_Scale_list.append(prefsBottom);
  m_Scale_list.append(prefsLeft);
  m_Scale_list.append(prefsTop);
  m_Scale_list.append(prefsRight);

  m_lstScales->setSizePolicy(
      QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
  m_lstScales->setIconSize(image0.size());

  // calculate a sensible width for the items list
  // (default QListWidget size is 256 which looks too big)
  QFontMetrics fm(m_lstScales->font());
  int width = 32;

  for (int i = 0; i < m_lstScales->count(); i++) {
    if (fm.width(m_lstScales->item(i)->text()) > width) {
      width = fm.width(m_lstScales->item(i)->text());
    }
  }

  m_lstScales->setMaximumWidth(m_lstScales->iconSize().width() + width + 50);
  // resize the list to the maximum width
  m_lstScales->resize(m_lstScales->maximumWidth(), m_lstScales->height());

  m_generalDialog->addTab(m_scalesPage, tr("Scale"));
  connect(m_lstScales, SIGNAL(currentRowChanged(int)), m_scalePrefsArea,
          SLOT(setCurrentIndex(int)));
}

/**initialises the axes tab
 *
 */
void AxesDialog::initAxesPage() {
  m_axesPage = new QWidget();
  axesLayout = new QHBoxLayout(m_axesPage);
  // axes page
  QPixmap image4((const char **)image4_data);
  QPixmap image5((const char **)image5_data);
  QPixmap image6((const char **)image6_data);
  QPixmap image7((const char **)image7_data);

  m_lstAxes = new QListWidget();
  m_axesPrefsArea = new QStackedLayout();
  axesLayout->addWidget(m_lstAxes);
  axesLayout->addLayout(m_axesPrefsArea);

  QListWidgetItem *listBottom = new QListWidgetItem(image4, tr("Bottom"));
  QListWidgetItem *listLeft = new QListWidgetItem(image5, tr("Left"));
  QListWidgetItem *listTop = new QListWidgetItem(image6, tr("Top"));
  QListWidgetItem *listRight = new QListWidgetItem(image7, tr("Right"));

  AxisDetails *prefsBottom = new AxisDetails(m_app, m_graph, QwtPlot::xBottom);
  AxisDetails *prefsLeft = new AxisDetails(m_app, m_graph, QwtPlot::yLeft);
  AxisDetails *prefsTop = new AxisDetails(m_app, m_graph, QwtPlot::xTop);
  AxisDetails *prefsRight = new AxisDetails(m_app, m_graph, QwtPlot::yRight);

  m_axesPrefsArea->addWidget(prefsBottom);
  m_axesPrefsArea->addWidget(prefsLeft);
  m_axesPrefsArea->addWidget(prefsTop);
  m_axesPrefsArea->addWidget(prefsRight);

  m_lstAxes->addItem(listBottom);
  m_lstAxes->addItem(listLeft);
  m_lstAxes->addItem(listTop);
  m_lstAxes->addItem(listRight);

  m_Axis_list.append(prefsBottom);
  m_Axis_list.append(prefsLeft);
  m_Axis_list.append(prefsTop);
  m_Axis_list.append(prefsRight);

  m_lstAxes->setIconSize(image6.size());
  m_lstAxes->setSizePolicy(
      QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

  // calculate a sensible width for the items list
  // (default QListWidget size is 256 which looks too big)
  QFontMetrics fm(m_lstAxes->font());
  int width = 32;

  for (int i = 0; i < m_lstAxes->count(); i++) {
    if (fm.width(m_lstAxes->item(i)->text()) > width) {
      width = fm.width(m_lstAxes->item(i)->text());
    }
  }

  m_lstAxes->setMaximumWidth(m_lstAxes->iconSize().width() + width + 50);
  // resize the list to the maximum width
  m_lstAxes->resize(m_lstAxes->maximumWidth(), m_lstAxes->height());

  m_generalDialog->addTab(m_axesPage, tr("Axis"));
  connect(m_lstAxes, SIGNAL(currentRowChanged(int)), m_axesPrefsArea,
          SLOT(setCurrentIndex(int)));
}

/**initialises the grid tab
 *
 */
void AxesDialog::initGridPage() {
  Grid *grd = dynamic_cast<Grid *>(m_graph->plotWidget()->grid());
  if (!grd) {
    return;
  }
  m_gridPage = new QWidget();

  QPixmap image2((const char **)image2_data);
  QPixmap image3((const char **)image3_data);

  QVBoxLayout *gridPageLayout = new QVBoxLayout(m_gridPage);
  QGroupBox *rightBox = new QGroupBox(QString());
  m_gridPrefsArea = new QStackedLayout(rightBox);

  m_lstGrid = new QListWidget();
  QHBoxLayout *topBox = new QHBoxLayout();
  topBox->addWidget(m_lstGrid);
  topBox->addWidget(rightBox);

  m_lstGrid->addItem(new QListWidgetItem(image3, tr("Horizontal")));
  m_lstGrid->addItem(new QListWidgetItem(image2, tr("Vertical")));
  m_lstGrid->setIconSize(image3.size());
  // calculate a sensible width for the items list
  // (default QListWidget size is 256 which looks too big)
  QFontMetrics fm(m_lstGrid->font());
  int width = 32, i;
  for (i = 0; i < m_lstGrid->count(); i++) {
    if (fm.width(m_lstGrid->item(i)->text()) > width) {
      width = fm.width(m_lstGrid->item(i)->text());
    }
  }

  m_lstGrid->setMaximumWidth(m_lstGrid->iconSize().width() + width + 50);
  // resize the list to the maximum width
  m_lstGrid->resize(m_lstGrid->maximumWidth(), m_lstGrid->height());

  GridDetails *prefsHor = new GridDetails(m_app, m_graph, 0);
  GridDetails *prefsVert = new GridDetails(m_app, m_graph, 1);

  m_gridPrefsArea->addWidget(prefsHor);
  m_gridPrefsArea->addWidget(prefsVert);

  m_Grid_list.append(prefsHor);
  m_Grid_list.append(prefsVert);

  QGridLayout *bottombox = new QGridLayout();

  bottombox->addWidget(new QLabel(tr("Apply To")), 0, 0, Qt::AlignRight);
  m_cmbApplyGridFormat = new QComboBox();
  m_cmbApplyGridFormat->insertItem(0, tr("This Layer"));
  m_cmbApplyGridFormat->insertItem(1, tr("This Window"));
  m_cmbApplyGridFormat->insertItem(2, tr("All Windows"));
  bottombox->addWidget(m_cmbApplyGridFormat, 0, 1, Qt::AlignLeft);

  m_chkAntialiseGrid = new QCheckBox(tr("An&tialised"));
  bottombox->addWidget(m_chkAntialiseGrid, 0, 2, Qt::AlignLeft);
  m_chkAntialiseGrid->setChecked(
      grd->testRenderHint(QwtPlotItem::RenderAntialiased));
  m_chkAntialiseGrid->setToolTip("Attempts to remove visual artifacts caused "
                                 "by plot resolution giving a smoother plot");

  gridPageLayout->addLayout(topBox);
  gridPageLayout->addLayout(bottombox);

  m_generalDialog->addTab(m_gridPage, tr("Grid"));

  // showGridOptions(m_lstGrid->currentRow());

  // grid page slot connections
  connect(m_lstGrid, SIGNAL(currentRowChanged(int)), m_gridPrefsArea,
          SLOT(setCurrentIndex(int)));
  connect(m_chkAntialiseGrid, SIGNAL(clicked()), prefsHor, SLOT(setModified()));
  connect(m_chkAntialiseGrid, SIGNAL(clicked()), prefsVert,
          SLOT(setModified()));
}

/**initialises the general tab
 *
 */
void AxesDialog::initGeneralPage() {
  m_generalPage = new QWidget();

  Plot *p = m_graph->plotWidget();

  QGroupBox *boxAxes = new QGroupBox(tr("Axes"));
  QGridLayout *boxAxesLayout = new QGridLayout(boxAxes);

  m_chkBackbones = new QCheckBox();
  m_chkBackbones->setText(tr("Draw backbones"));
  boxAxesLayout->addWidget(m_chkBackbones, 0, 0);

  boxAxesLayout->addWidget(new QLabel(tr("Line Width")), 1, 0);
  m_spnAxesLinewidth = new QSpinBox();
  m_spnAxesLinewidth->setRange(1, 100);
  boxAxesLayout->addWidget(m_spnAxesLinewidth, 1, 1);

  boxAxesLayout->addWidget(new QLabel(tr("Major ticks length")), 2, 0);
  m_spnMajorTicksLength = new QSpinBox();
  m_spnMajorTicksLength->setRange(0, 1000);
  boxAxesLayout->addWidget(m_spnMajorTicksLength, 2, 1);

  boxAxesLayout->addWidget(new QLabel(tr("Minor ticks length")), 3, 0);
  m_spnMinorTicksLength = new QSpinBox();
  m_spnMinorTicksLength->setRange(0, 1000);
  boxAxesLayout->addWidget(m_spnMinorTicksLength, 3, 1);
  boxAxesLayout->setRowStretch(4, 1);

  QHBoxLayout *mainLayout = new QHBoxLayout(m_generalPage);
  mainLayout->addWidget(boxAxes);

  m_generalDialog->addTab(m_generalPage, tr("General"));

  m_chkBackbones->setChecked(m_graph->axesBackbones());
  m_spnAxesLinewidth->setValue(p->axesLinewidth());
  m_spnMinorTicksLength->setValue(p->minorTickLength());
  m_spnMajorTicksLength->setValue(p->majorTickLength());

  connect(m_spnMajorTicksLength, SIGNAL(valueChanged(int)), this,
          SLOT(changeMajorTicksLength(int)));
  connect(m_spnMinorTicksLength, SIGNAL(valueChanged(int)), this,
          SLOT(changeMinorTicksLength(int)));

  connect(m_spnAxesLinewidth, SIGNAL(valueChanged(int)), this,
          SLOT(setModified()));
  connect(m_spnMajorTicksLength, SIGNAL(valueChanged(int)), this,
          SLOT(setModified()));
  connect(m_spnMinorTicksLength, SIGNAL(valueChanged(int)), this,
          SLOT(setModified()));
  connect(m_chkBackbones, SIGNAL(clicked()), this, SLOT(setModified()));
  connect(m_chkAntialiseGrid, SIGNAL(clicked()), this, SLOT(setModified()));
  connect(m_cmbApplyGridFormat, SIGNAL(currentIndexChanged(int)), this,
          SLOT(setModified()));
}

/**sets the flag that shows the general tab has been modified
 *
 */
void AxesDialog::setModified() { m_generalModified = true; }

/** sets the Minimum length of major ticks
 *
 *  @param minLength :: the current value of m_spnMinorTicksLength
 */
void AxesDialog::changeMinorTicksLength(int minLength) {
  m_spnMajorTicksLength->setMinimum(minLength);
}

/** sets the Maximum length of minor ticks
 *
 *  @param majLength :: the current value of m_spnMajorTicksLength
 */
void AxesDialog::changeMajorTicksLength(int majLength) {
  m_spnMinorTicksLength->setMaximum(majLength);
}

/** makes sure the selected axis on the scale and axis tabs are the same
 *
 *  @param page :: the tab that has just been switched to
 */
void AxesDialog::pageChanged(QWidget *page) {
  if (m_lastPage == m_scalesPage && page == m_axesPage) {
    m_lstAxes->setCurrentRow(m_lstScales->currentRow());
    m_lastPage = page;
  } else if (m_lastPage == m_axesPage && page == m_scalesPage) {
    m_lstScales->setCurrentRow(m_lstAxes->currentRow());
    m_lastPage = page;
  }
}

/**updates the grid overlay on the graph
 *
 */
void AxesDialog::updateGrid() {
  bool antiAlias = m_chkAntialiseGrid->isChecked();
  switch (m_cmbApplyGridFormat->currentIndex()) {
  case 0: {
    for (auto gridItr = m_Grid_list.begin(); gridItr != m_Grid_list.end();
         gridItr++) {
      if ((*gridItr)->modified()) {
        (*gridItr)->apply(m_graph->plotWidget()->grid(), antiAlias);
        m_graph->replot();
        m_graph->notifyChanges();
      }
    }
    break;
  }
  case 1: {
    MultiLayer *plot = m_graph->multiLayer();
    if (!plot) {
      return;
    }
    for (auto gridItr = m_Grid_list.begin(); gridItr != m_Grid_list.end();
         gridItr++) {
      QList<Graph *> layers = plot->layersList();
      foreach (Graph *g, layers) {
        if (g->isPiePlot()) {
          continue;
        }
        (*gridItr)->apply(g->plotWidget()->grid(), antiAlias);
        g->replot();
      }
    }
    plot->applicationWindow()->modifiedProject();
    break;
  }
  case 2: {
    if (!m_app) {
      return;
    }
    QList<MdiSubWindow *> windows = m_app->windowsList();
    foreach (MdiSubWindow *w, windows) {
      if (auto multi = dynamic_cast<MultiLayer *>(w)) {
        QList<Graph *> layers = multi->layersList();
        foreach (Graph *g, layers) {
          if (g->isPiePlot()) {
            continue;
          }
          for (auto gridItr = m_Grid_list.begin(); gridItr != m_Grid_list.end();
               gridItr++) {
            (*gridItr)->apply(g->plotWidget()->grid(), antiAlias, true);
            g->replot();
          }
        }
      }
    }
    m_app->modifiedProject();
    break;
  }
  }
}

/**applies the changes throughout the entire dialog to the graph
 *
 */
bool AxesDialog::pressToGraph() {
  // Check if all tabs and axes are valid first
  for (auto axisItr = m_Axis_list.begin(); axisItr != m_Axis_list.end();
       axisItr++) {
    if (!((*axisItr)->valid())) {
      g_log.warning("Axis options are invalid!");
      return false;
    }
  }

  for (auto scaleItr = m_Scale_list.begin(); scaleItr != m_Scale_list.end();
       scaleItr++) {
    if (!((*scaleItr)->valid())) {
      g_log.warning("Scale options are invalid!");
      return false;
    }
  }

  updateGrid();

  for (auto axisItr = m_Axis_list.begin(); axisItr != m_Axis_list.end();
       axisItr++) {
    (*axisItr)->apply();
  }

  for (auto scaleItr = m_Scale_list.begin(); scaleItr != m_Scale_list.end();
       scaleItr++) {
    (*scaleItr)->apply();
  }

  if (m_generalModified) {
    m_graph->changeTicksLength(m_spnMinorTicksLength->value(),
                               m_spnMajorTicksLength->value());
    m_graph->drawAxesBackbones(m_chkBackbones->isChecked());
    m_graph->setAxesLinewidth(m_spnAxesLinewidth->value());
    m_generalModified = false;
  }
  m_graph->replot();

  return true;
}
