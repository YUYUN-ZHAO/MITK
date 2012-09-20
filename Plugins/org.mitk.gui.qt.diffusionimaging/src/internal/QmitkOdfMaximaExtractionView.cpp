/*=========================================================================

 Program:   Medical Imaging & Interaction Toolkit
 Language:  C++
 Date:      $Date: 2010-03-31 16:40:27 +0200 (Mi, 31 Mrz 2010) $
 Version:   $Revision: 21975 $

 Copyright (c) German Cancer Research Center, Division of Medical and
 Biological Informatics. All rights reserved.
 See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 =========================================================================*/

//misc
#define _USE_MATH_DEFINES
#include <math.h>

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "QmitkOdfMaximaExtractionView.h"

// MITK
#include <mitkImageCast.h>
#include <mitkFiberBundleX.h>
#include <mitkImage.h>
#include <mitkDiffusionImage.h>
#include <mitkImageToItk.h>
#include <mitkTensorImage.h>

// ITK
#include <itkVectorImage.h>
#include <itkOdfMaximaExtractionFilter.h>
#include <itkFiniteDiffOdfMaximaExtractionFilter.h>
#include <itkFslPeakImageConverter.h>
#include <itkFslShCoefficientImageConverter.h>
#include <itkDiffusionTensorPrincipalDirectionImageFilter.h>

const std::string QmitkOdfMaximaExtractionView::VIEW_ID = "org.mitk.views.odfmaximaextractionview";
using namespace mitk;

QmitkOdfMaximaExtractionView::QmitkOdfMaximaExtractionView()
    : QmitkFunctionality()
    , m_Controls( 0 )
    , m_MultiWidget( NULL )
{

}

// Destructor
QmitkOdfMaximaExtractionView::~QmitkOdfMaximaExtractionView()
{

}


void QmitkOdfMaximaExtractionView::CreateQtPartControl( QWidget *parent )
{
    // build up qt view, unless already done
    if ( !m_Controls )
    {
        // create GUI widgets from the Qt Designer's .ui file
        m_Controls = new Ui::QmitkOdfMaximaExtractionViewControls;
        m_Controls->setupUi( parent );

        connect((QObject*) m_Controls->m_StartTensor, SIGNAL(clicked()), (QObject*) this, SLOT(StartTensor()));
        connect((QObject*) m_Controls->m_StartFiniteDiff, SIGNAL(clicked()), (QObject*) this, SLOT(StartFiniteDiff()));
        connect((QObject*) m_Controls->m_GenerateImageButton, SIGNAL(clicked()), (QObject*) this, SLOT(GenerateImage()));
        connect((QObject*) m_Controls->m_ConvertFromFsl, SIGNAL(clicked()), (QObject*) this, SLOT(ConvertPeaksFromFsl()));
        connect((QObject*) m_Controls->m_ConvertShFromFsl, SIGNAL(clicked()), (QObject*) this, SLOT(ConvertShCoeffsFromFsl()));
    }
}

void QmitkOdfMaximaExtractionView::UpdateGui()
{
    m_Controls->m_GenerateImageButton->setEnabled(false);
    m_Controls->m_StartFiniteDiff->setEnabled(false);
    m_Controls->m_StartTensor->setEnabled(false);
    m_Controls->m_CoeffImageFrame->setEnabled(false);

    if (!m_ImageNodes.empty() || !m_TensorImageNodes.empty())
    {
        if (!m_TensorImageNodes.empty())
        {
            m_Controls->m_DwiFibLabel->setText(m_TensorImageNodes.front()->GetName().c_str());
            m_Controls->m_StartTensor->setEnabled(true);
        }
        else
        {
            m_Controls->m_DwiFibLabel->setText(m_ImageNodes.front()->GetName().c_str());
            m_Controls->m_StartFiniteDiff->setEnabled(true);
            m_Controls->m_GenerateImageButton->setEnabled(true);
            m_Controls->m_CoeffImageFrame->setEnabled(true);
        }
    }
    else
        m_Controls->m_DwiFibLabel->setText("-");


    if (m_ImageNodes.empty())
    {
        m_Controls->m_ConvertFromFsl->setEnabled(false);
        m_Controls->m_ConvertShFromFsl->setEnabled(false);
    }
    else
    {
        m_Controls->m_ConvertFromFsl->setEnabled(true);
        m_Controls->m_ConvertShFromFsl->setEnabled(true);
    }

    if (!m_BinaryImageNodes.empty())
    {
        m_Controls->m_MaskLabel->setText(m_BinaryImageNodes.front()->GetName().c_str());
    }
    else
    {
        m_Controls->m_MaskLabel->setText("-");
    }
}

template<int shOrder>
void QmitkOdfMaximaExtractionView::TemplatedConvertShCoeffsFromFsl(mitk::Image* mitkImg)
{
    typedef itk::FslShCoefficientImageConverter< float, shOrder > FilterType;
    typedef mitk::ImageToItk< itk::Image< float, 4 > > CasterType;
    CasterType::Pointer caster = CasterType::New();
    caster->SetInput(mitkImg);
    caster->Update();

    typename FilterType::Pointer filter = FilterType::New();
    filter->SetInputImage(caster->GetOutput());
    filter->GenerateData();
    typename FilterType::QballImageType::Pointer itkQbi = filter->GetQballImage();
    typename FilterType::CoefficientImageType::Pointer itkCi = filter->GetCoefficientImage();

    {
        mitk::Image::Pointer img = mitk::Image::New();
        img->InitializeByItk( itkCi.GetPointer() );
        img->SetVolume( itkCi->GetBufferPointer() );
        DataNode::Pointer node = DataNode::New();
        node->SetData(img);
        node->SetName("FSL_ShCoefficientImage");
        GetDataStorage()->Add(node);
    }

    {
        mitk::QBallImage::Pointer img = mitk::QBallImage::New();
        img->InitializeByItk( itkQbi.GetPointer() );
        img->SetVolume( itkQbi->GetBufferPointer() );
        DataNode::Pointer node = DataNode::New();
        node->SetData(img);
        node->SetName("FSL_QballImage");
        GetDataStorage()->Add(node);
    }
}

void QmitkOdfMaximaExtractionView::ConvertShCoeffsFromFsl()
{
    if (m_ImageNodes.empty())
        return;
    mitk::Image::Pointer mitkImg = dynamic_cast<mitk::Image*>(m_ImageNodes.at(0)->GetData());
    if (mitkImg->GetDimension()!=4)
    {
        MITK_INFO << "wrong image type (need 4 dimensions)";
        return;
    }

    int nrCoeffs = mitkImg->GetLargestPossibleRegion().GetSize()[3];
    // solve bx² + cx + d = 0 = shOrder² + 2*shOrder + 2-2*neededCoeffs;
    int c=3, d=2-2*nrCoeffs;
    double D = c*c-4*d;
    int shOrder;
    if (D>0)
    {
        shOrder = (-c+sqrt(D))/2.0;
        if (shOrder<0)
            shOrder = (-c-sqrt(D))/2.0;
    }
    else if (D==0)
        shOrder = -c/2.0;

    MITK_INFO << "using SH-order " << shOrder;

    switch (shOrder)
    {
    case 4:
        TemplatedConvertShCoeffsFromFsl<4>(mitkImg);
        break;
    case 6:
        TemplatedConvertShCoeffsFromFsl<6>(mitkImg);
        break;
    case 8:
        TemplatedConvertShCoeffsFromFsl<8>(mitkImg);
        break;
    case 10:
        TemplatedConvertShCoeffsFromFsl<10>(mitkImg);
        break;
    case 12:
        TemplatedConvertShCoeffsFromFsl<12>(mitkImg);
        break;
    default:
        MITK_INFO << "SH-order " << shOrder << " not supported";
    }
}

void QmitkOdfMaximaExtractionView::ConvertPeaksFromFsl()
{
    if (m_ImageNodes.empty())
        return;

    typedef itk::Image< float, 4 > ItkImageType;
    typedef itk::FslPeakImageConverter< float > FilterType;
    FilterType::Pointer filter = FilterType::New();
    FilterType::InputType::Pointer inputVec = FilterType::InputType::New();
    mitk::Geometry3D::Pointer geom;

    for (int i=0; i<m_ImageNodes.size(); i++)
    {
        mitk::Image::Pointer mitkImg = dynamic_cast<mitk::Image*>(m_ImageNodes.at(i)->GetData());
        geom = mitkImg->GetGeometry();
        typedef mitk::ImageToItk< FilterType::InputImageType > CasterType;
        CasterType::Pointer caster = CasterType::New();
        caster->SetInput(mitkImg);
        caster->Update();
        FilterType::InputImageType::Pointer itkImg = caster->GetOutput();
        inputVec->InsertElement(inputVec->Size(), itkImg);
    }

    filter->SetInputImages(inputVec);
    filter->GenerateData();

    mitk::Vector3D outImageSpacing = geom->GetSpacing();
    float maxSpacing = 1;
    if(outImageSpacing[0]>outImageSpacing[1] && outImageSpacing[0]>outImageSpacing[2])
        maxSpacing = outImageSpacing[0];
    else if (outImageSpacing[1] > outImageSpacing[2])
        maxSpacing = outImageSpacing[1];
    else
        maxSpacing = outImageSpacing[2];

    mitk::FiberBundleX::Pointer directions = filter->GetOutputFiberBundle();
    directions->SetGeometry(geom);
    DataNode::Pointer node = DataNode::New();
    node->SetData(directions);
    node->SetName("FSL_VectorField");
    node->SetProperty("Fiber2DSliceThickness", mitk::FloatProperty::New(maxSpacing));
    node->SetProperty("Fiber2DfadeEFX", mitk::BoolProperty::New(false));
    GetDataStorage()->Add(node);

    typedef FilterType::DirectionImageContainerType DirectionImageContainerType;
    DirectionImageContainerType::Pointer container = filter->GetDirectionImageContainer();
    for (int i=0; i<container->Size(); i++)
    {
        ItkDirectionImage3DType::Pointer itkImg = container->GetElement(i);
        mitk::Image::Pointer img = mitk::Image::New();
        img->InitializeByItk( itkImg.GetPointer() );
        img->SetVolume( itkImg->GetBufferPointer() );
        DataNode::Pointer node = DataNode::New();
        node->SetData(img);
        QString name(m_ImageNodes.at(i)->GetName().c_str());
        name += "_Direction";
        name += QString::number(i+1);
        node->SetName(name.toStdString().c_str());
        GetDataStorage()->Add(node);
    }
}

void QmitkOdfMaximaExtractionView::GenerateImage()
{
    if (!m_ImageNodes.empty())
        GenerateDataFromDwi();
}

void QmitkOdfMaximaExtractionView::StartTensor()
{
    if (m_TensorImageNodes.empty())
        return;

    typedef itk::DiffusionTensorPrincipalDirectionImageFilter< float, float > MaximaExtractionFilterType;
    MaximaExtractionFilterType::Pointer filter = MaximaExtractionFilterType::New();

    mitk::Geometry3D::Pointer geometry;
    try{
        TensorImage::Pointer img = dynamic_cast<TensorImage*>(m_TensorImageNodes.at(0)->GetData());
        ItkTensorImage::Pointer itkImage = ItkTensorImage::New();
        CastToItkImage<ItkTensorImage>(img, itkImage);
        filter->SetInput(itkImage);
        geometry = img->GetGeometry();
    }
    catch(itk::ExceptionObject &e)
    {
        MITK_INFO << "wrong image type: " << e.what();
        throw e;
    }

    if (!m_BinaryImageNodes.empty())
    {
        ItkUcharImgType::Pointer itkMaskImage = ItkUcharImgType::New();
        Image::Pointer mitkMaskImg = dynamic_cast<Image*>(m_BinaryImageNodes.at(0)->GetData());
        CastToItkImage<ItkUcharImgType>(mitkMaskImg, itkMaskImage);
        filter->SetMaskImage(itkMaskImage);
    }

    if (m_Controls->m_NormalizationBox->currentIndex()==0)
        filter->SetNormalizeVectors(false);

    filter->Update();

    if (m_Controls->m_OutputDirectionImagesBox->isChecked())
    {
        MaximaExtractionFilterType::OutputImageType::Pointer itkImg = filter->GetOutput();
        mitk::Image::Pointer img = mitk::Image::New();
        img->InitializeByItk( itkImg.GetPointer() );
        img->SetVolume( itkImg->GetBufferPointer() );
        DataNode::Pointer node = DataNode::New();
        node->SetData(img);
        QString name(m_TensorImageNodes.at(0)->GetName().c_str());
        name += "_PrincipalDirection";
        node->SetName(name.toStdString().c_str());
        GetDataStorage()->Add(node);
    }

    if (m_Controls->m_OutputNumDirectionsBox->isChecked())
    {
        ItkUcharImgType::Pointer numDirImage = filter->GetNumDirectionsImage();
        mitk::Image::Pointer image2 = mitk::Image::New();
        image2->InitializeByItk( numDirImage.GetPointer() );
        image2->SetVolume( numDirImage->GetBufferPointer() );
        DataNode::Pointer node2 = DataNode::New();
        node2->SetData(image2);
        QString name(m_TensorImageNodes.at(0)->GetName().c_str());
        name += "_NumDirections";
        node2->SetName(name.toStdString().c_str());
        GetDataStorage()->Add(node2);
    }

    if (m_Controls->m_OutputVectorFieldBox->isChecked())
    {
        mitk::Vector3D outImageSpacing = geometry->GetSpacing();
        float minSpacing = 1;
        if(outImageSpacing[0]<outImageSpacing[1] && outImageSpacing[0]<outImageSpacing[2])
            minSpacing = outImageSpacing[0];
        else if (outImageSpacing[1] < outImageSpacing[2])
            minSpacing = outImageSpacing[1];
        else
            minSpacing = outImageSpacing[2];

        mitk::FiberBundleX::Pointer directions = filter->GetOutputFiberBundle();
        directions->SetGeometry(geometry);
        DataNode::Pointer node = DataNode::New();
        node->SetData(directions);
        QString name(m_TensorImageNodes.at(0)->GetName().c_str());
        name += "_VectorField";
        node->SetName(name.toStdString().c_str());
        node->SetProperty("Fiber2DSliceThickness", mitk::FloatProperty::New(minSpacing));
        node->SetProperty("Fiber2DfadeEFX", mitk::BoolProperty::New(false));
        GetDataStorage()->Add(node);
    }
}

template<int shOrder>
void QmitkOdfMaximaExtractionView::StartMaximaExtraction()
{
    typedef itk::FiniteDiffOdfMaximaExtractionFilter< float, shOrder, 20242 > MaximaExtractionFilterType;
    typename MaximaExtractionFilterType::Pointer filter = MaximaExtractionFilterType::New();

    mitk::Geometry3D::Pointer geometry;
    try{
        Image::Pointer img = dynamic_cast<Image*>(m_ImageNodes.at(0)->GetData());
        typedef ImageToItk< typename MaximaExtractionFilterType::CoefficientImageType > CasterType;
        typename CasterType::Pointer caster = CasterType::New();
        caster->SetInput(img);
        caster->Update();
        filter->SetInput(caster->GetOutput());
        geometry = img->GetGeometry();
    }
    catch(itk::ExceptionObject &e)
    {
        MITK_INFO << "wrong image type: " << e.what();
        throw;
    }

    filter->SetMaxNumPeaks(m_Controls->m_MaxNumPeaksBox->value());
    filter->SetPeakThreshold(m_Controls->m_PeakThresholdBox->value());
    filter->SetAbsolutePeakThreshold(m_Controls->m_AbsoluteThresholdBox->value());

    if (!m_BinaryImageNodes.empty())
    {
        ItkUcharImgType::Pointer itkMaskImage = ItkUcharImgType::New();
        Image::Pointer mitkMaskImg = dynamic_cast<Image*>(m_BinaryImageNodes.at(0)->GetData());
        CastToItkImage<ItkUcharImgType>(mitkMaskImg, itkMaskImage);
        filter->SetMaskImage(itkMaskImage);
    }

    switch (m_Controls->m_NormalizationBox->currentIndex())
    {
    case 0:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::NO_NORM);
        break;
    case 1:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::MAX_VEC_NORM);
        break;
    case 2:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::SINGLE_VEC_NORM);
        break;
    }

    filter->Update();

    if (m_Controls->m_OutputDirectionImagesBox->isChecked())
    {
        typedef typename MaximaExtractionFilterType::ItkDirectionImageContainer ItkDirectionImageContainer;
        typename ItkDirectionImageContainer::Pointer container = filter->GetDirectionImageContainer();
        for (int i=0; i<container->Size(); i++)
        {
            typename MaximaExtractionFilterType::ItkDirectionImage::Pointer itkImg = container->GetElement(i);
            mitk::Image::Pointer img = mitk::Image::New();
            img->InitializeByItk( itkImg.GetPointer() );
            img->SetVolume( itkImg->GetBufferPointer() );
            DataNode::Pointer node = DataNode::New();
            node->SetData(img);
            QString name(m_ImageNodes.at(0)->GetName().c_str());
            name += "_Direction";
            name += QString::number(i+1);
            node->SetName(name.toStdString().c_str());
            GetDataStorage()->Add(node);
        }
    }

    if (m_Controls->m_OutputNumDirectionsBox->isChecked())
    {
        ItkUcharImgType::Pointer numDirImage = filter->GetNumDirectionsImage();
        mitk::Image::Pointer image2 = mitk::Image::New();
        image2->InitializeByItk( numDirImage.GetPointer() );
        image2->SetVolume( numDirImage->GetBufferPointer() );
        DataNode::Pointer node2 = DataNode::New();
        node2->SetData(image2);
        QString name(m_ImageNodes.at(0)->GetName().c_str());
        name += "_NumDirections";
        node2->SetName(name.toStdString().c_str());
        GetDataStorage()->Add(node2);
    }

    if (m_Controls->m_OutputVectorFieldBox->isChecked())
    {
        mitk::Vector3D outImageSpacing = geometry->GetSpacing();
        float minSpacing = 1;
        if(outImageSpacing[0]<outImageSpacing[1] && outImageSpacing[0]<outImageSpacing[2])
            minSpacing = outImageSpacing[0];
        else if (outImageSpacing[1] < outImageSpacing[2])
            minSpacing = outImageSpacing[1];
        else
            minSpacing = outImageSpacing[2];

        mitk::FiberBundleX::Pointer directions = filter->GetOutputFiberBundle();
        directions->SetGeometry(geometry);
        DataNode::Pointer node = DataNode::New();
        node->SetData(directions);
        QString name(m_ImageNodes.at(0)->GetName().c_str());
        name += "_VectorField";
        node->SetName(name.toStdString().c_str());
        node->SetProperty("Fiber2DSliceThickness", mitk::FloatProperty::New(minSpacing));
        node->SetProperty("Fiber2DfadeEFX", mitk::BoolProperty::New(false));
        GetDataStorage()->Add(node);
    }
}

void QmitkOdfMaximaExtractionView::StartFiniteDiff()
{
    if (m_ImageNodes.empty())
        return;

    switch (m_Controls->m_ShOrderBox->currentIndex())
    {
    case 0:
        StartMaximaExtraction<2>();
        break;
    case 1:
        StartMaximaExtraction<4>();
        break;
    case 2:
        StartMaximaExtraction<6>();
        break;
    case 3:
        StartMaximaExtraction<8>();
        break;
    case 4:
        StartMaximaExtraction<10>();
        break;
    case 5:
        StartMaximaExtraction<12>();
        break;
    }
}

void QmitkOdfMaximaExtractionView::GenerateDataFromDwi()
{
    typedef itk::OdfMaximaExtractionFilter< float > MaximaExtractionFilterType;
    MaximaExtractionFilterType::Pointer filter = MaximaExtractionFilterType::New();

    mitk::Geometry3D::Pointer geometry;
    if (!m_ImageNodes.empty())
    {
        try{
            Image::Pointer img = dynamic_cast<Image*>(m_ImageNodes.at(0)->GetData());
            typedef ImageToItk< MaximaExtractionFilterType::CoefficientImageType > CasterType;
            CasterType::Pointer caster = CasterType::New();
            caster->SetInput(img);
            caster->Update();
            filter->SetShCoeffImage(caster->GetOutput());
            geometry = img->GetGeometry();
        }
        catch(itk::ExceptionObject &e)
        {
            MITK_INFO << "wrong image type: " << e.what();
            return;
        }
    }
    else
        return;

    filter->SetMaxNumPeaks(m_Controls->m_MaxNumPeaksBox->value());
    filter->SetPeakThreshold(m_Controls->m_PeakThresholdBox->value());

    if (!m_BinaryImageNodes.empty())
    {
        ItkUcharImgType::Pointer itkMaskImage = ItkUcharImgType::New();
        Image::Pointer mitkMaskImg = dynamic_cast<Image*>(m_BinaryImageNodes.at(0)->GetData());
        CastToItkImage<ItkUcharImgType>(mitkMaskImg, itkMaskImage);
        filter->SetMaskImage(itkMaskImage);
    }

    switch (m_Controls->m_NormalizationBox->currentIndex())
    {
    case 0:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::NO_NORM);
        break;
    case 1:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::MAX_VEC_NORM);
        break;
    case 2:
        filter->SetNormalizationMethod(MaximaExtractionFilterType::SINGLE_VEC_NORM);
        break;
    }

    filter->SetUseAdaptiveStepWidth(true);
    filter->GenerateData();

    ItkUcharImgType::Pointer numDirImage = filter->GetNumDirectionsImage();

    if (m_Controls->m_OutputDirectionImagesBox->isChecked())
    {
        typedef MaximaExtractionFilterType::ItkDirectionImageContainer ItkDirectionImageContainer;
        ItkDirectionImageContainer::Pointer container = filter->GetDirectionImageContainer();
        for (int i=0; i<container->Size(); i++)
        {
            MaximaExtractionFilterType::ItkDirectionImage::Pointer itkImg = container->GetElement(i);
            mitk::Image::Pointer img = mitk::Image::New();
            img->InitializeByItk( itkImg.GetPointer() );
            img->SetVolume( itkImg->GetBufferPointer() );
            DataNode::Pointer node = DataNode::New();
            node->SetData(img);
            QString name(m_ImageNodes.at(0)->GetName().c_str());
            name += "_Direction";
            name += QString::number(i+1);
            node->SetName(name.toStdString().c_str());
            GetDataStorage()->Add(node);
        }
    }

    if (m_Controls->m_OutputNumDirectionsBox->isChecked())
    {
        mitk::Image::Pointer image2 = mitk::Image::New();
        image2->InitializeByItk( numDirImage.GetPointer() );
        image2->SetVolume( numDirImage->GetBufferPointer() );
        DataNode::Pointer node = DataNode::New();
        node->SetData(image2);
        QString name(m_ImageNodes.at(0)->GetName().c_str());
        name += "_NumDirections";
        node->SetName(name.toStdString().c_str());
        GetDataStorage()->Add(node);
    }

    if (m_Controls->m_OutputVectorFieldBox->isChecked())
    {
        mitk::Vector3D outImageSpacing = geometry->GetSpacing();
        float minSpacing = 1;
        if(outImageSpacing[0]<outImageSpacing[1] && outImageSpacing[0]<outImageSpacing[2])
            minSpacing = outImageSpacing[0];
        else if (outImageSpacing[1] < outImageSpacing[2])
            minSpacing = outImageSpacing[1];
        else
            minSpacing = outImageSpacing[2];

        mitk::FiberBundleX::Pointer directions = filter->GetOutputFiberBundle();
        directions->SetGeometry(geometry);
        DataNode::Pointer node = DataNode::New();
        node->SetData(directions);
        QString name(m_ImageNodes.at(0)->GetName().c_str());
        name += "_VectorField";
        node->SetName(name.toStdString().c_str());
        node->SetProperty("Fiber2DSliceThickness", mitk::FloatProperty::New(minSpacing));
        node->SetProperty("Fiber2DfadeEFX", mitk::BoolProperty::New(false));
        GetDataStorage()->Add(node);
    }
}

void QmitkOdfMaximaExtractionView::StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget)
{
    m_MultiWidget = &stdMultiWidget;
}

void QmitkOdfMaximaExtractionView::StdMultiWidgetNotAvailable()
{
    m_MultiWidget = NULL;
}

void QmitkOdfMaximaExtractionView::OnSelectionChanged( std::vector<mitk::DataNode*> nodes )
{
    if ( !this->IsVisible() )
    {
        // do nothing if nobody wants to see me :-(
        return;
    }

    m_BinaryImageNodes.clear();
    m_ImageNodes.clear();
    m_QballImageNodes.clear();
    m_TensorImageNodes.clear();

    // iterate all selected objects, adjust warning visibility
    for( std::vector<mitk::DataNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it )
    {
        mitk::DataNode::Pointer node = *it;

        if ( node.IsNotNull() && dynamic_cast<mitk::QBallImage*>(node->GetData()) )
        {
            m_QballImageNodes.push_back(node);
        }
        else if ( node.IsNotNull() && dynamic_cast<mitk::TensorImage*>(node->GetData()) )
        {
            m_TensorImageNodes.push_back(node);
        }
        else if( node.IsNotNull() && dynamic_cast<mitk::Image*>(node->GetData()) )
        {
            bool isBinary = false;
            node->GetPropertyValue<bool>("binary", isBinary);
            if (isBinary)
                m_BinaryImageNodes.push_back(node);
            else
                m_ImageNodes.push_back(node);
        }
    }

    UpdateGui();
}
