.. _IDA-Structure-ref:

IndirectDataAnalysis File Structure
===================================

The many levels of inheritance in the indirect data analysis codebase can be confusing.

Tabs
----

- IndirectTab

  - IndirectDataAnalysisTab

    - IndirectDataAnalysisElwinTab
    - IndirectDataAnalysisIqtTab
    - IndirectFitAnalysisTab

      - IndirectDataAnalysisConvFitTab
      - IndirectDataAnalysisFqFitTab
      - IndirectDataAnalysisIqtFitTab
      - IndirectDataAnalysisMSDFitTab

Fitting Models
--------------
- IndirectFittingModel

  - ConvFitModel
  - FqFitModel
  - IqtFitModel
  - MSDFitModel

Fit Data Presenter
------------------
These are the single/multiple tabs for loading and selecting data

- IndirectFitDataPresenter (Used by MSDFit and IqtFit)
  - ConvFitDataPresenter
  - FqFitDataPresenter

Data Table Presenter
--------------------
These control the multiple loading tables

- IndirectDataTablePresenter (Used by MSDFit and IqtFit)

  - ConvFitDataTablePresenter
  - FqFitDataTablePresenter

AddWorkspaceDialog
------------------
This is the Dialogue for adding workspaces in multiple mode

- IAddWorkspaceDialog

  - ConvFitAddWorkspaceDialog
  - FqFitAddWorkspaceDialog
  - IndirectAddWorkspaceDialog (Used by MSDFit and IqtFit)

Function Template Browser
-------------------------
These are the fit function templates for each fitting tab, switched out for the full version when `show full function` is checked

- FunctionTemplateBrowser

  - ConvTemplateBrowser
  - IqtTemplateBrowser
  - SingleFunctionTemplateBrowser (Used by MSDFit and FqFit)

Function Template Model
-----------------------
- IFunctionModel

  - ConvFunctionModel
  - IqtFunctionModel
  - FunctionModel
    - SingleFunctionTemplateModel

Function Template Presenter
---------------------------
- ConvTemplatePresenter
- IqtTemplatePresenter
- SingleFunctionTemplatePresenter