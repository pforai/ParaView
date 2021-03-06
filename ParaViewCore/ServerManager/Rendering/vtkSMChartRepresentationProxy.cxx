/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartRepresentationProxy.h"

#include "vtkChartRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMChartSeriesListDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::vtkSMChartRepresentationProxy()
{
  this->SetSIClassName("vtkSIChartRepresentationProxy");
  this->SetDefaults = true;
}

//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::~vtkSMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkChartRepresentation* vtkSMChartRepresentationProxy::GetRepresentation()
{
  this->CreateVTKObjects();
  return vtkChartRepresentation::SafeDownCast(
    this->GetClientSideObject());
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkSMChartRepresentationProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  vtkSMProxy* optionsProxy = this->GetSubProxy("PlotOptions");
  if (optionsProxy)
    {
    const char* names[] = {
      "Input",
      "CompositeDataSetIndex",
      "AttributeType",
      NULL
    };
    for (int cc=0; names[cc] != NULL; cc++)
      {
      vtkSMProperty* src = this->GetProperty(names[cc]);
      vtkSMProperty* dest = optionsProxy->GetProperty(names[cc]);
      if (src && dest)
        {
        this->LinkProperty(src, dest);
        }
      }
    }

  const char* setDefaults = element->GetAttribute("set_defaults");
  if (setDefaults)
    {
    this->SetDefaults = strcmp(setDefaults, "false") == 0 ? false : true;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::SetPropertyModifiedFlag(
  const char* name, int flag)
{
  if (name && strcmp(name, "Input") == 0)
    {
    vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
        helper.GetAsProxy(cc));
      if (input)
        {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(
          helper.GetOutputPort(cc));
        if (!esProxy)
          {
          vtkErrorMacro("Input proxy does not support selection extraction.");
          }
        else
          {
          vtkSMProxy* selectionReprProxy =
            this->GetSubProxy("SelectionRepresentation");
          if (selectionReprProxy)
            {
            // We use these internal properties since we need to add consumer dependecy
            // on this proxy so that MarkModified() is called correctly.
            vtkSMPropertyHelper(selectionReprProxy, "SelectionInput").Set(esProxy, 1);
            selectionReprProxy->UpdateVTKObjects();
            }
          }
        }
      }
    }
  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::ResetPropertiesToDefault()
{
  this->Superclass::ResetPropertiesToDefault();

  if (this->SetDefaults)
    {
    vtkSMProperty* useIndex = this->GetProperty("UseIndexForXAxis");
    vtkSMProperty* xarrayName = this->GetProperty("XArrayName");

    if (useIndex && xarrayName)
      {
      vtkSMPropertyHelper helper(xarrayName);
      const char* value = helper.GetAsString();
      const char** known_names =
        vtkSMChartSeriesListDomain::GetKnownSeriesNames();
      for (int cc=0; known_names[cc] != NULL && value != NULL; cc++)
        {
        if (strcmp(known_names[cc], value) == 0)
          {
          vtkSMPropertyHelper(useIndex).Set(0);
          this->UpdateProperty("UseIndexForXAxis");
          return;
          }
        }
      }

    vtkSMPropertyHelper(useIndex).Set(1);
    this->UpdateProperty("UseIndexForXAxis");
    }
}
