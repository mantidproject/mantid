#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"
#include "MantidQtAPI/MdSettings.h"
#include "boost/shared_ptr.hpp"

#include <vector>
#include <pqRenderView.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMViewProxy.h>
#include <vtkCommand.h>
#include <vtkCallbackCommand.h>
#include <vtkSmartPointer.h>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {

      QColor BackgroundRgbProvider::currentBackgroundColor = QColor(84,89,109);

      BackgroundRgbProvider::BackgroundRgbProvider(boost::shared_ptr<MantidQt::API::MdSettings> settings) : mdSettings(settings)
      {
      };

      BackgroundRgbProvider::~BackgroundRgbProvider()
      {
        // Update the settings before exiting
        updateLastSessionBackgroundColor();
      };

      std::vector<double> BackgroundRgbProvider::getRgb(bool viewSwitched)
      {
        // Get the rgb setting from the config file
        std::vector<double> userSettingRgb = getRgbFromSetting(viewSwitched);
        
        // Normalize the entries to 256
        userSettingRgb[0] = userSettingRgb[0]/255.0;
        userSettingRgb[1] = userSettingRgb[1]/255.0;
        userSettingRgb[2] = userSettingRgb[2]/255.0;

        return userSettingRgb;
      }

      std::vector<double> BackgroundRgbProvider::getRgbFromSetting(bool viewSwitched)
      {
        // Set the mantid default here
        std::vector<double> background;
        QColor userBackground;

        if (viewSwitched)
        {
          // Update the settings
          updateLastSessionBackgroundColor();

          userBackground = mdSettings->getLastSessionBackgroundColor();
        }
        else
        {
          if (mdSettings->getUsageLastSession())
          {
            userBackground = mdSettings->getLastSessionBackgroundColor();
          }
          else
          {
            // Select the user setting as the background color and make the user setting the last session color
            userBackground= mdSettings->getUserSettingBackgroundColor();

            mdSettings->setLastSessionBackgroundColor(userBackground);
          }

          // Need to make sure that the static variable is initialized correctly, else it will show a black background
          currentBackgroundColor = userBackground;
        }

        // Get the background
        int rVal;
        int gVal;
        int bVal;

        if (userBackground.isValid())
        {
          rVal = userBackground.red();
          gVal = userBackground.green();
          bVal = userBackground.blue();
        }
        else
        {
          // Set the default
          QColor defaultBackgroundColor = mdSettings->getDefaultBackgroundColor();
          rVal = defaultBackgroundColor.red();
          gVal = defaultBackgroundColor.green();
          bVal = defaultBackgroundColor.blue();
        }

        background.push_back(static_cast<double>(rVal));
        background.push_back(static_cast<double>(gVal));
        background.push_back(static_cast<double>(bVal));

        return background;
      }

      void BackgroundRgbProvider::updateLastSessionBackgroundColor()
      {
        mdSettings->setLastSessionBackgroundColor(currentBackgroundColor);
      }

      void BackgroundRgbProvider::setBackgroundColor(pqRenderView* view, bool viewSwitched)
      {
        std::vector<double> backgroundRgb = getRgb(viewSwitched);

        vtkSMDoubleVectorProperty* background = vtkSMDoubleVectorProperty::SafeDownCast(view->getViewProxy()->GetProperty("Background"));

        background->SetElements3(backgroundRgb[0],backgroundRgb[1],backgroundRgb[2]);

        view->resetCamera();
      }

      void BackgroundRgbProvider::observe(pqRenderView* view)
      {
        // For more information http://www.vtk.org/Wiki/VTK/Tutorials/Callbacks
        vtkSmartPointer<vtkCallbackCommand> backgroundColorChangeCallback = vtkSmartPointer<vtkCallbackCommand>::New();

        backgroundColorChangeCallback->SetCallback(backgroundColorChangeCallbackFunction);

        view->getViewProxy()->GetProperty("Background")->AddObserver(vtkCommand::ModifiedEvent, backgroundColorChangeCallback);
      }

      void BackgroundRgbProvider::backgroundColorChangeCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId), void* vtkNotUsed(clientData), void* vtkNotUsed(callData))
      {
        // Extract the background color and persist it 
        vtkSMDoubleVectorProperty* background =vtkSMDoubleVectorProperty::SafeDownCast(caller);

        int numberOfElements = background->GetNumberOfElements();
        double* elements = background->GetElements();

        if (numberOfElements >= 3)
        {
          double r = elements[0]*255.0;
          double g = elements[1]*255.0;
          double b = elements[2]*255.0;

          int red = static_cast<int>(r);
          int green = static_cast<int>(g);
          int blue = static_cast<int>(b);

          currentBackgroundColor = QColor(red,green,blue);
        }
      }
    }
  }
}