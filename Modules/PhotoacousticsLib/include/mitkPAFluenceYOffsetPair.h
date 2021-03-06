/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef MITKPAFLUENCEYOFFSETPAIR_H
#define MITKPAFLUENCEYOFFSETPAIR_H

#include <MitkPhotoacousticsLibExports.h>
#include <itkMacro.h>

#include "mitkCommon.h"
#include "itkLightObject.h"
#include <mitkPAVolume.h>

namespace mitk
{
  namespace pa {
    class MITKPHOTOACOUSTICSLIB_EXPORT FluenceYOffsetPair : public itk::LightObject
    {
    public:
      mitkClassMacroItkParent(FluenceYOffsetPair, itk::LightObject);
      mitkNewMacro2Param(Self, mitk::pa::Volume::Pointer, double);

      FluenceYOffsetPair(const FluenceYOffsetPair::Pointer other);

      const FluenceYOffsetPair &operator=(const FluenceYOffsetPair::Pointer other)
      {
        if (this == other)
          return *this;

        m_FluenceValue = other->GetFluenceValue();
        m_YOffsetInCentimeters = other->GetYOffsetInCentimeters();

        return *this;
      }

      mitk::pa::Volume::Pointer GetFluenceValue() const;
      double GetYOffsetInCentimeters() const;

    private:
      FluenceYOffsetPair(mitk::pa::Volume::Pointer fluenceValue, double yOffset);

      ~FluenceYOffsetPair() override;

      mitk::pa::Volume::Pointer m_FluenceValue;
      double m_YOffsetInCentimeters;
    };
  }
}

#endif // MITKPHOTOACOUSTICCOMPOSEDVOLUME_H
