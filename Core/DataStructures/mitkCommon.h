/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#ifdef _MSC_VER
// This warns about truncation to 255 characters in debug/browse info
#pragma warning (disable : 4786)
#endif

//add only those headers here that are really necessary for all classes!
#include "itkObject.h"

//#include "itkRegion.h"
//#include "vecmath.h"
//#include "itkTransform.h"
//#include "itkProcessObject.h"
//#include "ipPic.h"
//#include "vtkImageData.h"
//#include "vtkImageReslice.h"
//
typedef  unsigned int MapperSlotId;
//typedef	Tree<mitk::DataTreeNode::Pointer> DataTree;
//typedef	TreeIterator<mitk::DataTreeNode::Pointer> DataTreeIteratorClone;

#define mitkClassMacro(className,SuperClassName) \
	typedef className				Self; \
	typedef SuperClassName			Superclass; \
	typedef itk::SmartPointer<Self> Pointer; \
	typedef itk::SmartPointer<const Self>  ConstPointer; \
    itkTypeMacro(className,SuperClassName)
/**
* Macro for Constructors with one parameter for classes derived from itk::Lightobject
**/
#define mitkNewMacro1Param(classname,type) \
static Pointer New(type _arg) \
{ \
  Pointer smartPtr = new classname ( _arg ); \
  smartPtr->UnRegister(); \
  return smartPtr; \
} \

/**
* Macro for Constructors with two parameters for classes derived from itk::Lightobject
**/
#define mitkNewMacro2Param(classname,typea,typeb) \
static Pointer New(typea _arga, typeb _argb) \
{ \
  Pointer smartPtr = new classname ( _arga, _argb ); \
  smartPtr->UnRegister(); \
  return smartPtr; \
} \

/**
* Macro for Constructors with three parameters for classes derived from itk::Lightobject
**/
#define mitkNewMacro3Param(classname,typea,typeb,typec) \
static Pointer New(typea _arga, typeb _argb, typec _argc) \
{ \
  Pointer smartPtr = new classname ( _arga, _argb, _argc ); \
  smartPtr->UnRegister(); \
  return smartPtr; \
} \

/**
* Macro for Constructors with three parameters for classes derived from itk::Lightobject
**/
#define mitkNewMacro4Param(classname,typea,typeb,typec,typed) \
static Pointer New(typea _arga, typeb _argb, typec _argc, typed _argd) \
{ \
  Pointer smartPtr = new classname ( _arga, _argb, _argc, _argd ); \
  smartPtr->UnRegister(); \
  return smartPtr; \
} \


/** Get a smart const pointer to an object.  Creates the member 
 * Get"name"() (e.g., GetPoints()). */
#define mitkGetObjectMacroConst(name,type) \
  virtual type * Get##name () const \
  { \
    itkDebugMacro("returning " #name " address " << this->m_##name ); \
    return this->m_##name.GetPointer(); \
  } 

/** provide a macro for adding MS specific __declspec(dllexport/-import)
 *  to classes.
 *  This is needed for the export of symbols, when you build a DLL. Then write
 *  
 *    class MITK_EXPORT ClassName : public SomeClass {};
 */
#if defined(WIN32)
  #if defined(BUILDING_MITK_DLL)
    #define MITK_EXPORT __declspec(dllexport)
  #elif defined(IMPORTING_MITK_DLL)
    #define MITK_EXPORT __declspec(dllimport)
  #else
    #define MITK_EXPORT
  #endif
#else
  #define MITK_EXPORT
#endif

