#ifndef MDEW_REBINNING_PRESENTER_H
#define MDEW_REBINNING_PRESENTER_H

#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/MDRebinningPresenter.h"
#include "MantidVatesAPI/RebinningActionManager.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetToGeometry.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"

#include <vtkDataSet.h>
#include <vtkFieldData.h>

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDEWRebinningPresenter, concrete MDRebinningPresenter using centre piece rebinning directly on MDEWs producing Histogrammed MDWs.
    @author Owen Arnold, Tessella plc
    @date 10/08/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    template<typename ViewType>
    class DLLExport MDEWRebinningPresenter : public MDRebinningPresenter
    {
    public:

      /*----------------------------------- MDRebinningPresenter methods  -------------------------------------*/

      virtual void updateModel();

      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& eventHandler);

      virtual const std::string& getAppliedGeometryXML() const;

      bool hasTDimensionAvailable() const;

      std::vector<double> getTimeStepValues() const;

       /*-----------------------------------End MDRebinningPresenter methods -------------------------------------*/
      
      MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view);
      
      virtual ~MDEWRebinningPresenter();

    private:

      std::string findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id);
      std::string findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id);
      void persistReductionKnowledge(vtkDataSet* out_ds, const RebinningKnowledgeSerializer& xmlGenerator, const char* id);
      std::string extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const;

      ///Parser used to process input vtk to extract metadata.
      vtkDataSetToGeometry m_inputParser;
      ///Input vtk dataset.
      vtkDataSet* m_input;
      ///Request, encapsulating priorisation of requests made for rebinning/redrawing.
      boost::scoped_ptr<RebinningActionManager> m_request;
      ///The view of this MVP pattern.
      ViewType* m_view;
      ///Maximum threshold
      signal_t m_maxThreshold;
      ///Minimum threshold
      signal_t m_minThreshold;
      ///The current timestep.
      double m_timestep;
      ///The workspace geometry. Cached value.
      mutable std::string m_wsGeometry;
      ///Serializer of rebinning 
      RebinningKnowledgeSerializer m_serializer;
    };

    template<typename ViewType>
    MDEWRebinningPresenter<ViewType>::MDEWRebinningPresenter(vtkDataSet* input, RebinningActionManager* request, ViewType* view) :
    m_inputParser(input),
      m_input(input), 
      m_request(request), 
      m_view(view), 
      m_maxThreshold(0),
      m_minThreshold(0),
      m_timestep(0),
      m_wsGeometry("")
    {
      vtkFieldData* fd = input->GetFieldData();
      if(NULL == fd || NULL == fd->GetArray(XMLDefinitions::metaDataId().c_str()))
      {
        throw std::logic_error("Rebinning operations require Rebinning Metadata");
      }

      vtkDataSetToGeometry parser(input);
      parser.execute();

      using Mantid::Geometry::MDGeometryBuilderXML;
      using Mantid::Geometry::StrictDimensionPolicy;
      MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;

      Mantid::Geometry::VecIMDDimension_sptr dimensions =parser.getAllDimensions();
      DimensionVec::iterator it = dimensions.begin();
      for(;it != dimensions.end(); ++it)
      {
        xmlBuilder.addOrdinaryDimension(*it);
      }
      if(parser.hasXDimension())
      {
        xmlBuilder.addXDimension(parser.getXDimension());
      }
      if(parser.hasYDimension())
      {
        xmlBuilder.addYDimension(parser.getYDimension());
      }
      if(parser.hasZDimension())
      {
        xmlBuilder.addZDimension(parser.getZDimension());
      }
      if(parser.hasTDimension())
      {
        xmlBuilder.addTDimension(parser.getTDimension());
      }

      //Apply the geometry.
      m_serializer.setGeometryXML(xmlBuilder.create());
      //Apply the workspace name after extraction from the input xml.
      m_serializer.setWorkspaceName( findExistingWorkspaceName(m_input, XMLDefinitions::metaDataId().c_str()));
      //Apply the workspace location after extraction from the input xml.
      m_serializer.setWorkspaceLocation( findExistingWorkspaceLocation(m_input, XMLDefinitions::metaDataId().c_str()));

    }

    template<typename ViewType>
    MDEWRebinningPresenter<ViewType>::~MDEWRebinningPresenter()
    {
    }

    template<typename ViewType>
    void MDEWRebinningPresenter<ViewType>::updateModel()
    {
      if(m_view->getTimeStep() != m_timestep)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_timestep = m_view->getTimeStep();
      }
      if(m_view->getMaxThreshold() != m_maxThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_maxThreshold = m_view->getMaxThreshold();
      }
      if(m_view->getMinThreshold() != m_minThreshold)
      {
        m_request->ask(RecalculateVisualDataSetOnly);
        m_minThreshold = m_view->getMinThreshold();
      }
      m_view->getApplyClip();
     

      m_serializer.setImplicitFunction(Mantid::API::ImplicitFunction_sptr(new Mantid::MDAlgorithms::CompositeImplicitFunction)); //TODO CHANGE!

      if(m_view->getAppliedGeometryXML() != m_serializer.getWorkspaceGeometry())
      {
        m_request->ask(RecalculateAll);
        m_serializer.setGeometryXML( m_view->getAppliedGeometryXML() );
      }
    }

    /**
   Mantid properties for rebinning algorithm require formatted information.
   @param dimension : dimension to extract property value for.
   @return true available, false otherwise.
 */
    template<typename ViewType>
std::string MDEWRebinningPresenter<ViewType>::extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const
{
  double min = dimension->getMinimum();
  double max = dimension->getMaximum();
  size_t nbins = dimension->getNBins();
  std::string id = dimension->getDimensionId();
  return boost::str(boost::format("%s, %f, %f, %d") %id %min %max % nbins);
}

template<typename ViewType>
vtkDataSet* MDEWRebinningPresenter<ViewType>::execute(vtkDataSetFactory* factory, ProgressAction& eventHandler)
{
  using namespace Mantid::API;
  if(RecalculateAll == m_request->action())
  {

    std::string wsName = m_serializer.getWorkspaceName();

    Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
    sourceGeometry.execute();

    Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
    hist_alg.initialize();

    hist_alg.setPropertyValue("InputWorkspace", wsName);
    std::string id; 
    if(sourceGeometry.hasXDimension())
    {
      hist_alg.setPropertyValue("DimX",  extractFormattedPropertyFromDimension(sourceGeometry.getXDimension())); 
    }
    if(sourceGeometry.hasYDimension())
    {
      hist_alg.setPropertyValue("DimY",  extractFormattedPropertyFromDimension(sourceGeometry.getYDimension())); 
    }
    if(sourceGeometry.hasZDimension())
    {
      hist_alg.setPropertyValue("DimZ",  extractFormattedPropertyFromDimension(sourceGeometry.getTDimension())); 
    }
    if(sourceGeometry.hasTDimension())
    {
      hist_alg.setPropertyValue("DimT",  extractFormattedPropertyFromDimension(sourceGeometry.getTDimension())); 
    }
    hist_alg.setPropertyValue("OutputWorkspace", "Histo_ws");

    Poco::NObserver<ProgressAction, Mantid::API::Algorithm::ProgressNotification> observer(eventHandler, &ProgressAction::handler);
    //Add observer.
    hist_alg.addObserver(observer);
    //Run the rebinning algorithm.
    hist_alg.execute();
    //Remove observer
    hist_alg.removeObserver(observer);
  }

  Mantid::API::Workspace_sptr result=Mantid::API::AnalysisDataService::Instance().retrieve("Histo_ws");

  factory->initialize(result);

  //vtkUnstructuredGrid* temp = static_cast<vtkUnstructuredGrid*>(factory->create());
  vtkDataSet* temp = factory->create();

  persistReductionKnowledge(temp, this->m_serializer, XMLDefinitions::metaDataId().c_str());
  m_request->reset();
  return temp;
}


    template<typename ViewType>
    const std::string& MDEWRebinningPresenter<ViewType>::getAppliedGeometryXML() const
    {
      return m_serializer.getWorkspaceGeometry();
    }

    template<typename ViewType>
    bool MDEWRebinningPresenter<ViewType>::hasTDimensionAvailable() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_serializer.getWorkspaceGeometry());
      sourceGeometry.execute();
      return sourceGeometry.getTDimension();
    }

    template<typename ViewType>
    std::vector<double> MDEWRebinningPresenter<ViewType>::getTimeStepValues() const
    {
      Mantid::Geometry::MDGeometryXMLParser sourceGeometry(m_view->getAppliedGeometryXML());
      sourceGeometry.execute();

      double min = sourceGeometry.getTDimension()->getMinimum();
      double max = sourceGeometry.getTDimension()->getMaximum();
      unsigned int nBins = static_cast<int>(sourceGeometry.getTDimension()->getNBins() );
      double increment = (max - min) / nBins;
      std::vector<double> timeStepValues(nBins);
      for (unsigned int i = 0; i < nBins; i++)
      {
        timeStepValues[i] = min + (i * increment);
      }
      return timeStepValues;
    }

    template<typename ViewType>
    std::string MDEWRebinningPresenter<ViewType>::findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id)
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      FieldDataToMetadata convert;
      std::string xmlString = convert(inputDataSet->GetFieldData(), id);

      Poco::XML::DOMParser pParser;
      Poco::XML::Document* pDoc = pParser.parseString(xmlString);
      Poco::XML::Element* pRootElem = pDoc->documentElement();
      Poco::XML::Element* wsNameElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceNameElementName());
      if(wsNameElem == NULL)
      {
        throw std::runtime_error("The element containing the workspace name must be present.");
      }
      return wsNameElem->innerText();
    }

    template<typename ViewType>
    std::string MDEWRebinningPresenter<ViewType>::findExistingWorkspaceLocation(vtkDataSet *inputDataSet, const char* id)
    {
      using Mantid::Geometry::MDGeometryXMLDefinitions;
      FieldDataToMetadata convert;
      std::string xmlString = convert(inputDataSet->GetFieldData(), id);

      Poco::XML::DOMParser pParser;
      Poco::XML::Document* pDoc = pParser.parseString(xmlString);
      Poco::XML::Element* pRootElem = pDoc->documentElement();
      Poco::XML::Element* wsLocationElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceLocationElementName());
      if(wsLocationElem == NULL)
      {
        throw std::runtime_error("The element containing the workspace location must be present.");
      }
      return wsLocationElem->innerText();
    }

    template<typename ViewType>
    void MDEWRebinningPresenter<ViewType>::persistReductionKnowledge(vtkDataSet* out_ds, const
      RebinningKnowledgeSerializer& xmlGenerator, const char* id)
    {
      vtkFieldData* fd = vtkFieldData::New();

      MetadataToFieldData convert;
      convert(fd, xmlGenerator.createXMLString().c_str(), id);

      out_ds->SetFieldData(fd);
      fd->Delete();
    }

  }
}

#endif