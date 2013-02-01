/*=============================================================================

  Library: CppMicroServices

  Copyright (c) German Cancer Research Center,
    Division of Medical and Biological Informatics

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include <usModuleContext.h>
#include <usGetModuleContext.h>
#include <usModuleRegistry.h>
#include <usModule.h>
#include <usModuleResource.h>
#include <usModuleResourceStream.h>

#include <usTestingConfig.h>

#include "usTestUtilSharedLibrary.h"
#include "usTestingMacros.h"

#include <assert.h>

#include <set>

US_USE_NAMESPACE

namespace {

std::string GetResourceContent(const ModuleResource& resource)
{
  std::string line;
  ModuleResourceStream rs(resource);
  std::getline(rs, line);
  return line;
}

struct ResourceComparator {
  bool operator()(const ModuleResource& mr1, const ModuleResource& mr2) const
  {
    return mr1 < mr2;
  }
};

void testResourcesWithStaticImport(Module* module)
{
  ModuleResource resource = module->GetResource("res.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(), "Check valid res.txt resource")
  std::string line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "dynamic resource", "Check dynamic resource content")

  resource = module->GetResource("dynamic.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(), "Check valid dynamic.txt resource")
  line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "dynamic", "Check dynamic resource content")

  resource = module->GetResource("static.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(), "Check valid static.txt resource")
  line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "static", "Check dynamic resource content")

  std::vector<ModuleResource> resources = module->FindResources("", "*.txt", true);
  std::stable_sort(resources.begin(), resources.end(), ResourceComparator());
  US_TEST_CONDITION(resources.size() == 4, "Check imported resource count")
  line = GetResourceContent(resources[0]);
  US_TEST_CONDITION(line == "dynamic", "Check dynamic.txt resource content")
  line = GetResourceContent(resources[1]);
  US_TEST_CONDITION(line == "dynamic resource", "Check res.txt (from importing module) resource content")
  line = GetResourceContent(resources[2]);
  US_TEST_CONDITION(line == "static resource", "Check res.txt (from imported module) resource content")
  line = GetResourceContent(resources[3]);
  US_TEST_CONDITION(line == "static", "Check static.txt (from importing module) resource content")
}

} // end unnamed namespace


int usStaticModuleResourceTest(int /*argc*/, char* /*argv*/[])
{
  US_TEST_BEGIN("StaticModuleResourceTest");

  ModuleContext* mc = GetModuleContext();
  assert(mc);

  SharedLibraryHandle libB("TestModuleB");

  try
  {
    libB.Load();
  }
  catch (const std::exception& e)
  {
    US_TEST_FAILED_MSG(<< "Load module exception: " << e.what())
  }

  Module* moduleB = ModuleRegistry::GetModule("TestModuleB Module");
  US_TEST_CONDITION_REQUIRED(moduleB != NULL, "Test for existing module TestModuleB")

  US_TEST_CONDITION(moduleB->GetName() == "TestModuleB Module", "Test module name")

  testResourcesWithStaticImport(moduleB);

  ModuleResource resource = moduleB->GetResource("static.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(), "Check valid static.txt resource")

  libB.Unload();

  US_TEST_CONDITION_REQUIRED(resource.IsValid() == false, "Check invalid static.txt resource")

  US_TEST_END()
}
