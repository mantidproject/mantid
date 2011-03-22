//--------------------------------------
// Includes
//--------------------------------------
#include "MantidColorMap.h"
#include "GLColor.h"

// std headers
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include <iostream>

//--------------------------------------
// Public member functions
//--------------------------------------
/**
 * Default
 */
MantidColorMap::MantidColorMap() : QwtColorMap(QwtColorMap::Indexed), m_scale_type(GraphOptions::Log10),
				     m_colors(0), m_num_colors(0)
{
  setupDefaultMap();
}

/**
 * Constructor with filename and type
 * @param 
 * @param type :: The scale type, currently Linear or Log10 
 */
MantidColorMap::MantidColorMap(const QString & filename, GraphOptions::ScaleType type) : 
  QwtColorMap(QwtColorMap::Indexed), m_scale_type(type), m_colors(0), m_num_colors(0)
{
  // Check and load default if this doesn't work
  if( !loadMap(filename) )
  {
    setupDefaultMap();
  }
}

/**
 * Destructor
 */
MantidColorMap::~MantidColorMap()
{
}

/**
 * Create a clone of the color map
 */
QwtColorMap* MantidColorMap::copy() const
{
  MantidColorMap *map = new MantidColorMap;
  *map = *this;

  return map;
}

/**
 * Change the scale type
 * @param type :: The new scale type
 */
void MantidColorMap::changeScaleType(GraphOptions::ScaleType type)
{
    m_scale_type = type;
}

/**
 * Load a color map from a file
 * @param filename :: The full path to the color map file
 */
bool MantidColorMap::loadMap(const QString & filename)
{
  std::ifstream cmapfile(filename.toStdString().c_str(),std::ios::in);
  if( !cmapfile.is_open() ) return false;

  // Reading directly to the color store will mean that if the file is corrupt 
  // at some point then I can't revert to my previous map. Using a QVector means
  // that copying the data at the end is not an expensive operation
  QVector<boost::shared_ptr<GLColor> > new_colormap;
  new_colormap.reserve(getLargestAllowedCIndex() + 1);

  float red(0.0f), green(0.0f), blue(0.0f);
  std::string line;
  short count(0);
  bool is_success(true);
  while ( std::getline(cmapfile, line) )
  {
    if(line.empty() || count == getLargestAllowedCIndex() + 1 ) break;
    std::stringstream reader(line);
    reader >> red >> green >> blue;
    if( reader )
    {
      new_colormap.push_back(boost::shared_ptr<GLColor>(new GLColor(red/255.0f, green/255.0f, blue/255.0f, 1.0)));
      ++count;
    }
    else 
    {
      is_success = false;
      break;
    }

  }
  cmapfile.close();

  if( is_success )
  {
    m_num_colors = count;
    m_colors = new_colormap;
  } 
  return is_success;
}

/**
 * Define a default color map to be used if a file is unavailable.
 */
void MantidColorMap::setupDefaultMap()
{
  // The coldfire map distrubuted with qtiplot is the default and putting this into a string with an @ separator was the
  // easiest way to construct it as it doesn't have a regular pattern so a loop wouldn't work.
  std::string colorstring = 
    "0 172 252@0 170 252@0 168 252@0 164 252@0 160 252@0 156 252@0 152 252@0 152 252@0 148 252@0 144 252@0 140 252@0 136 252"
    "@0 132 252@0 132 252@0 128 252@0 124 252@0 120 252@0 116 252@0 112 252@0 112 252@0 108 252@0 104 252@0 100 252@0  96 252"
    "@0  92 252@0  92 252@0  88 252@0  84 252@0  80 252@0  76 252@0  72 252@0  68 252@0  64 252@0  60 252@0  56 252@0  52 252"
    "@0  48 252@0  44 252@0  40 252@0  36 252@0  32 252@0  28 252@0  24 252@0  20 252@0  16 252@0  12 252@0   8 252@0   4 252"
    "@0   4 252@4   4 248@4   4 248@8   4 244@8   8 240@12   8 240@12   8 236@16   8 232@16  12 232@20  12 228@20  12 224"
    "@24  12 224@24  16 220@28  16 216@28  16 216@32  16 212@32  20 212@36  20 208@36  20 204@40  20 204@40  24 200@44  24 196"
    "@44  24 196@48  24 192@48  24 188@52  28 188@52  28 184@56  28 180@56  28 180@60  32 176@60  32 172@64  32 172@64  32 168"
    "@68  36 168@68  36 164@72  36 160@72  36 160@76  40 156@76  40 152@80  40 152@80  40 148@84  44 144@84  44 144@88  44 140"
    "@88  44 136@92  48 136@92  48 132@96  48 128@100  48 128@100  48 124@104  52 124@104  52 120@108  52 116@108  52 116"
    "@112  56 112@112  56 108@116  56 108@116  56 104@120  60 100@120  60 100@124  60  96@124  60  92@128  64  92@128  64  88"
    "@132  64  88@132  64  84@136  68  80@136  68  80@140  68  76@140  68  72@144  72  72@144  72  68@148  72  64@148  72  64"
    "@152  72  60@152  76  56@156  76  56@156  76  52@160  76  48@160  80  48@164  80  44@164  80  44@168  80  40@168  84  36"
    "@172  84  36@172  84  32@176  84  28@176  88  28@180  88  24@180  88  20@184  88  20@184  92  16@188  92  12@188  92  12"
    "@192  92   8@196  96   4@196  96   4@196 100   4@196 100   4@196 104   4@200 108   4@200 108   4@200 112   4@200 112   4"
    "@200 116   4@204 120   4@204 120   4@204 124   4@204 124   4@208 128   4@208 132   4@208 132   4@208 136   4@208 136   4"
    "@212 140   4@212 144   4@212 144   4@212 148   4@216 152   4@216 152   4@216 156   4@216 156   4@216 160   4@220 164   4"
    "@220 164   4@220 168   4@220 168   4@224 172   4@224 176   4@224 176   4@224 180   4@224 180   4@228 184   4@228 188   4"
    "@228 188   4@228 192   4@228 192   4@232 196   4@232 200   4@232 200   4@232 204   4@236 208   4@236 208   4@236 212   4"
    "@236 212   4@236 216   4@240 220   4@240 220   4@240 224   4@240 224   4@244 228   4@244 232   4@244 232   4@244 236   4"
    "@244 236   4@248 240   4@248 244   4@248 244   4@248 248   4@252 252   0@252 252 104@252 252 104@252 252 108@252 252 112"
    "@252 252 116@252 252 120@252 252 120@252 252 124@252 252 128@252 252 132@252 252 136@252 252 136@252 252 140@252 252 144"
    "@252 252 148@252 252 152@252 252 152@252 252 156@252 252 160@252 252 164@252 252 168@252 252 168@252 252 172@252 252 176"
    "@252 252 180@252 252 184@252 252 184@252 252 188@252 252 192@252 252 196@252 252 200@252 252 200@252 252 204@252 252 208"
    "@252 252 212@252 252 216@252 252 216@252 252 220@252 252 224@252 252 228@252 252 232@252 252 232@252 252 236@252 252 240"
    "@252 252 244@252 252 248@252 252 252@255 255 255@";

  m_colors.clear();
  m_num_colors = 256;
  std::stringstream colorstream(colorstring);
  float red(0.0f), green(0.0f), blue(0.0f);
  std::string line;
  while ( std::getline(colorstream, line, '@') )
  {
    std::stringstream reader(line);
    reader >> red >> green >> blue;
    m_colors.push_back(boost::shared_ptr<GLColor>(new GLColor(red/255.0f, green/255.0f, blue/255.0f, 1.0)));
  }  
}


/**
 * Normalize the value to the range[0,1]
 * @param interval :: The data range
 * @param value :: The data value
 * @returns The fraction along the given interval using the current scale type
 */
double MantidColorMap::normalize(const QwtDoubleInterval &interval, double value) const
{
  if( interval.isNull() || m_num_colors == 0 ) 
  {
    return -1.0;
  }

  const double width = interval.width();
  if( width <= 0.0 || value <= interval.minValue() )
  {
    return 0.0;
  }
  // nan numbers have the property that nan != nan, treat nan as being the maximum
  if ( value >= interval.maxValue() || value != value )
  {
    return 1.0;
  }
  double ratio(0.0);
  if( m_scale_type == GraphOptions::Linear)
  {
    ratio = (value - interval.minValue()) / width;
  }
  else
  {
    // Have to deal with the possibility that a user has entered 0 as a minimum
    double minValue = interval.minValue();
    if( minValue < 1e-08 )
    {
      minValue = 1.0;
    }
    ratio = std::log10(value/minValue)/std::log10(interval.maxValue()/minValue);
  }
  return ratio;
}


/**
 * Compute an rgb value for the given data value and interval
 * @param interval :: The data range
 * @param value :: Compute an RGB color for this data value
 */
QRgb MantidColorMap::rgb(const QwtDoubleInterval & interval, double value) const
{
  boost::shared_ptr<GLColor> col = getColor(colorIndex(interval, value));
  float r(0.0f), g(0.0f), b(0.0f), a(0.0f);
  col->get(r,g,b,a);
  return qRgb(r*255,g*255,b*255);
}


/**
 * Compute a color index
 * @param interval :: The data range
 * @param value :: The data value
 * @returns A color index as an unsigned character
 */

unsigned char MantidColorMap::colorIndex (const QwtDoubleInterval &interval, double value) const
{
  double fraction = normalize(interval, value);
  if( fraction < 0.0 )return static_cast<unsigned char>(0);

  short index = std::floor(fraction * m_num_colors);
  // If the ratio gives back 1 then we need to adjust the index down 1
  if( index >= m_num_colors )
  {
    index = m_num_colors - 1;
  }
  if( index < 0 )
  {
    index = 0;
  }
  return static_cast<unsigned char>(index);
}

/**
 * Compute a lookup table
 * @param interval :: The interval for the table to cover
 */
QVector<QRgb> MantidColorMap::colorTable(const QwtDoubleInterval & interval) const
{
  // Swicth to linear scaling when computing the lookup table
  GraphOptions::ScaleType current_type = m_scale_type;   
  m_scale_type = GraphOptions::Linear;

  short table_size = (m_num_colors > 1) ? m_num_colors : 2;
  QVector<QRgb> rgbtable(table_size);
  if( interval.isValid() )
  {
    const double step = interval.width() / table_size;
    for( short i = 0; i < table_size; ++i )
    {
      rgbtable[i] = rgb(interval, interval.minValue() + step*i);
    }
  }
  
  //Restore scaling type
  m_scale_type = current_type;
  return rgbtable;
}

/**
 * Returns a pointer to the GLColor object for the given id in the range 0 <= index < ncolors
 * @param An :: index within this color map's range.
 *
 */
boost::shared_ptr<GLColor> MantidColorMap::getColor(unsigned char index) const
{
  short ci = static_cast<short>(index);
  if( ci >= 0 && ci < m_num_colors )
  {
    return m_colors[ci];
  }

  // Otherwise return black
  return boost::shared_ptr<GLColor>(new GLColor(0.0f, 0.0f, 0.0f, 1.0f));
}
