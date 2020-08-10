/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "DeepLearningSegmentationTool.h"
#include <mitkToolManager.h>
#include <usGetModuleContext.h>
#include <mitkStandardFileLocations.h>
#include <mitkIPythonService.h>

mitk::DeepLearningSegmentationTool::DeepLearningSegmentationTool(std::string pythonFolder,
                                                                 std::string inputImageVarName,
                                                                 std::string pythonFileName,
                                                                 std::string outputImageVarName,
                                                                 ImageType imageType)
{
  m_PythonProjectPath = "Modules/DeepLearningSegmentation/"+pythonFolder;
  m_InputImageVarName = inputImageVarName;
  m_PythonFileName = pythonFileName;
  m_OutputImageVarName = outputImageVarName;
  m_ImageType = imageType;

  m_SegmentationRunning = false;
}

mitk::DeepLearningSegmentationTool::~DeepLearningSegmentationTool() {
}

//bool mitk::DeepLearningSegmentationTool::CanHandle(mitk::BaseData *referenceData) const
//{
//  if (referenceData == nullptr)
//    return false;
//
//  auto *image = dynamic_cast<mitk::Image *>(referenceData);
//  if (image == nullptr)
//    return false;
//
//  if (image->GetDimension() != 3)
//    return false;
//
//  return true;
//}

const char **mitk::DeepLearningSegmentationTool::GetXPM() const
{
  return nullptr;
}

void mitk::DeepLearningSegmentationTool::Activated()
{
  Superclass::Activated();
}

void mitk::DeepLearningSegmentationTool::Deactivated()
{
  Superclass::Deactivated();
}

mitk::LabelSetImage::Pointer mitk::DeepLearningSegmentationTool::DoSegmentation(std::string networkPath)
{
  m_SegmentationRunning = true;
  //get the input Image
  mitk::Image::Pointer input;
  try
  {
    input = GetInputImage();
  }
  catch(mitk::Exception &e)
  {
    MITK_ERROR << e.GetDescription();
    mitkThrow();
  }

  //Get the python microservice
  mitk::IPythonService::ForceLoadModule(); 
  us::ModuleContext *context = us::GetModuleContext();
  std::string filter = "(Name=PythonService)";
  auto m_PythonServiceRefs = context->GetServiceReferences<mitk::IPythonService>(filter);

  if (!m_PythonServiceRefs.empty())
  {
    mitk::IPythonService *m_PythonService =
      dynamic_cast<mitk::IPythonService *>(context->GetService<mitk::IPythonService>(m_PythonServiceRefs.front()));
    // set path to the Python code which should be executed
      try
      {
        std::vector<std::string> pathVector;
        pathVector.push_back(m_PythonProjectPath);
        m_PythonService->AddRelativeSearchDirs(pathVector);
      }
      catch (const mitk::Exception &e)
      {
        MITK_ERROR << e.GetDescription();
        mitkThrow() << "Error in setting the path to the Python code which should be executed";
        m_SegmentationRunning = false;
        return nullptr;
      }
   // set the path to the trained network
      try
      {
        std::string pathCommand = "network_path = '" + networkPath+"'";
        m_PythonService->Execute(pathCommand);
      }
      catch (const mitk::Exception &e)
      {
        MITK_ERROR << e.GetDescription();
        mitkThrow() << "Error in setting the network path";
        m_SegmentationRunning = false;
        return nullptr;
      }

      //set the input image
      try
      {
        if (m_ImageType==DeepLearningSegmentationTool::SimpleITKImage)
        {
          m_PythonService->CopyToPythonAsSimpleItkImage(input, m_InputImageVarName);
        }
        else if (m_ImageType==DeepLearningSegmentationTool::MITKImage)
        {
          m_PythonService->CopyMITKImageToPython(input, m_InputImageVarName);
        }
        else
        {
          mitkThrow() << "Unknown image type";
        }
      }
      catch (const mitk::Exception &e)
      {
        MITK_ERROR << e.GetDescription();
        mitkThrow() << "Error setting the input image";
        m_SegmentationRunning = false;
        return nullptr;
      }

        // execute Segmentation
      try
      {
        std::string fileName = mitk::StandardFileLocations::GetInstance()->FindFile(
          m_PythonFileName.c_str(), m_PythonProjectPath.c_str());
        m_PythonService->ExecuteScript(fileName);
      }
      catch (const mitk::Exception &e)
      {
        MITK_ERROR << e.GetDescription();
        mitkThrow() << "Error in executing python code";
        m_SegmentationRunning = false;
        return nullptr;
      }

      // get result
      try
      {
        mitk::Image::Pointer outputImage;
        if (m_ImageType == DeepLearningSegmentationTool::SimpleITKImage)
        {
          outputImage = m_PythonService->CopySimpleItkImageFromPython(m_OutputImageVarName);
        }
        else if (m_ImageType == DeepLearningSegmentationTool::MITKImage)
        {
          outputImage = m_PythonService->CopyMITKImageFromPython(m_OutputImageVarName);
        }
        else
        {
          mitkThrow() << "Unknown image type";
        }
        mitk::LabelSetImage::Pointer resultImage = mitk::LabelSetImage::New();
        resultImage->InitializeByLabeledImage(outputImage);
        resultImage->SetGeometry(input->GetGeometry());
        m_SegmentationRunning = false;
        outputImage->SetGeometry(input->GetGeometry());
        return resultImage;
      }
      catch (const mitk::Exception &e)
      {
        MITK_ERROR << e.GetDescription();
        mitkThrow() << "Error in getting the result";
        m_SegmentationRunning = false;
        return nullptr;
      }
  }
  else
  {
    mitkThrow() << "No service reference found";
  }
  m_SegmentationRunning = false;
  return nullptr;
}

mitk::DataStorage *mitk::DeepLearningSegmentationTool::GetDataStorage()
{
  return m_ToolManager->GetDataStorage();
  m_ToolManager->GetReferenceData(0);
}

mitk::DataNode *mitk::DeepLearningSegmentationTool::GetReferenceData()
{
  return m_ToolManager->GetReferenceData(0);
}
  
mitk::Image::Pointer mitk::DeepLearningSegmentationTool::GetInputImage()
  {
  mitk::DataNode::Pointer referenceData = m_ToolManager->GetReferenceData(0);
  mitk::Image::Pointer input = dynamic_cast<mitk::Image *>(referenceData->GetData());
  if (input.IsNull())
  {
    mitkThrow();
  }
  //unsigned int timestep = mitk::RenderingManager::GetInstance()->GetTimeNavigationController()->GetTime()->GetPos();
  //mitk::Image::ConstPointer input = Get3DImage(input, timestep);
  if (input.IsNull())
  {
    mitkThrow();
  }

  return input;
  }

bool mitk::DeepLearningSegmentationTool::IsSegmentationRunning() 
{
  return m_SegmentationRunning;
}