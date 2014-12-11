#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <vector>
#include <cctype>
#include <stdlib.h>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      namespace
      {
        /// static logger
        Mantid::Kernel::Logger g_log("BackgroundRgbProvider");
      }


      BackgroundRgbProvider::BackgroundRgbProvider():separator(",")
      {
        // Set the default color
        defaultBackground.push_back(84);
        defaultBackground.push_back(89);
        defaultBackground.push_back(109); 

        // Set the map for the background colors
        
        // Black
        std::string black = "BLACK";

        std::vector<double> blackValues;
        blackValues.push_back(0.0);
        blackValues.push_back(0.0);
        blackValues.push_back(0.0);

        backgroundMap.insert(std::pair<std::string, std::vector<double>>(black, blackValues));

        // White 
        std::string white = "WHITE";

        std::vector<double> whiteValues;
        whiteValues.push_back(255.0);
        whiteValues.push_back(255.0);
        whiteValues.push_back(255.0);

        backgroundMap.insert(std::pair<std::string, std::vector<double>>(white, whiteValues));

        // Grey
        std::string grey = "GREY";

        std::vector<double> greyValues;
        greyValues.push_back(160.0);
        greyValues.push_back(160.0);
        greyValues.push_back(160.0);

        backgroundMap.insert(std::pair<std::string, std::vector<double>>(grey, greyValues));
      }
      
      BackgroundRgbProvider::~BackgroundRgbProvider()
      {
      }
      
      std::vector<double> BackgroundRgbProvider::getRgb()
      {
        // Get the rgb setting from the config file
        std::vector<double> userSettingRgb =  getRbgFromPropertiesFile();
        
        // Normalize the entries to 256
        userSettingRgb[0] = userSettingRgb[0]/255.0;
        userSettingRgb[1] = userSettingRgb[1]/255.0;
        userSettingRgb[2] = userSettingRgb[2]/255.0;

        return userSettingRgb;
      }

      std::vector<double> BackgroundRgbProvider::getRbgFromPropertiesFile()
      {
        // Set the mantid default here
        std::vector<double> background;
        
        // Check in the Mantid.users.properties file if a default color map was specified
        std::string userBackground= Kernel::ConfigService::Instance().getVsiDefaultBackgroundColor();
        
        if (!userBackground.empty())
        {
          // Try to get comma separated values from the 
          std::vector<std::string> colorsNumeric = this->getCommaSeparatedEntries(userBackground);

          if (colorsNumeric.size() == 3)
          {
            background = this->getFromNumericSetting(colorsNumeric);
          }
          else 
          {
            background = this->getFromNameSetting(userBackground);
          }
        }
        else
        {
          background = this-> defaultBackground;
        }

        return background;
      }


      std::vector<std::string> BackgroundRgbProvider::getCommaSeparatedEntries(std::string background)
      {
        std::vector<std::string> colors;
        size_t pos = 0;

        std::string token;
       
        while ((pos = background.find(this->separator)) != std::string::npos)
        {
          token = background.substr(0, pos);

          colors.push_back(token);

          background.erase(0, pos + this->separator.length());
        }

        if (!background.empty())
        {
          colors.push_back(background);
        }

        return colors;
      }
      
      std::vector<double> BackgroundRgbProvider::getFromNumericSetting(std::vector<std::string> background)
      {
        std::vector<double> colors;

        if (background.size() == 3)
        {
          // Convert the string values to double values
          double entry;
          for (std::vector<std::string>::iterator it = background.begin(); it != background.end(); ++it)
          {
            entry = atof(it->c_str());

            // Make sure that the entry is valid -> 0 <= entry <= 255
            if (isNumeric(*it) && isValidNumericEntry(entry))
            {
              colors.push_back(entry);
            }
            else
            {
              // If an entry is invalid, then exit with the default setting
              g_log.warning() << "Warning: The VSI background color specified in the Mantid.user.profiles file is not valid. \n";

              return this->defaultBackground;
            }
          }
        }
        else 
        {
          // If the wrong number of columns is specifed
          g_log.warning() << "Warning: The VSI background color specified in the Mantid.user.profiles file is not valid. \n";
          colors = this->defaultBackground;
        }

        return colors;
      }

      bool BackgroundRgbProvider::isNumeric(std::string entry)
      {
        std::string::const_iterator it = entry.begin();

        // Currently we only allow integer entries
        while (it != entry.end() && std::isdigit(*it))
        {
          ++it;
        }

        if (!entry.empty() && it == entry.end())
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      bool BackgroundRgbProvider::isValidNumericEntry(double entry)
      {
        double lowerBound = 0.0;
        double upperBound = 255.0;

        if (entry >= lowerBound && entry <= upperBound)
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      std::vector<double> BackgroundRgbProvider::getFromNameSetting(std::string background)
      {
        // Convert to upper case
        std::transform(background.begin(), background.end(), background.begin(), toupper);

        // Check if exists
        if (backgroundMap.count(background) > 0)
        {
          return backgroundMap[background];
        }
        else
        {
          g_log.warning() << "Warning: The VSI background color specified in the Mantid.user.profiles file is not valid. \n";
          return this->defaultBackground;
        }
      }
    }
  }
}