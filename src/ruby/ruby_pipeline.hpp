//
// Copyright Copyright 2009-2021, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#pragma once

#include "entity/entity.hpp"
#include "device_model/device.hpp"
#include "device_model/data_item/data_item.hpp"

#include <rice/rice.hpp>
#include <rice/stl.hpp>
#include <ruby/thread.h>
#include "ruby_transform.hpp"

namespace mtconnect::ruby {
  using namespace mtconnect;
  using namespace pipeline;
  using namespace std;
  using namespace Rice;
  
  struct RubyPipeline {
    void create(Rice::Module &module)
    {
      m_transform =  define_class_under<Transform>(module, "Transform");
      m_rubyTransform =  define_class_under<RubyTransform, Transform>(module, "RubyTransform");
      m_pipelineContext =  define_class_under<pipeline::PipelineContext>(module, "PipelineContext");
      m_pipeline = define_class_under<pipeline::Pipeline>(module, "Pipeline");
    }
    
    void methods()
    {
      m_pipeline.define_method("splice_before", [](pipeline::Pipeline *p, const std::string name, pipeline::TransformPtr trans) {
          p->spliceBefore(name, trans);
        }, Arg("name"), Arg("transform")).
      define_method("splice_after", [](pipeline::Pipeline *p, const std::string name, pipeline::TransformPtr trans) {
          p->spliceAfter(name, trans);
        }, Arg("name"), Arg("transform")).
      define_method("first_after", [](pipeline::Pipeline *p, const std::string name, pipeline::TransformPtr trans) {
          p->firstAfter(name, trans);
        }, Arg("name"), Arg("transform")).
      define_method("last_after", [](pipeline::Pipeline *p, const std::string name, pipeline::TransformPtr trans) {
          p->lastAfter(name, trans);
        }, Arg("name"), Arg("transform")).
      define_method("run", [](pipeline::Pipeline *p, entity::EntityPtr entity) { p->run(entity); }, Arg("entity")).
      define_method("context", [](pipeline::Pipeline *p) { return p->getContext(); });
      
      m_transform.define_method("name", [](RubyTransform* t) {
        return t->getName(); });
      m_rubyTransform.define_constructor(Constructor<RubyTransform, const string>(), Arg("name"));
    }
    
    Data_Type<Transform> m_transform;
    Data_Type<RubyTransform> m_rubyTransform;
    Data_Type<Pipeline> m_pipeline;
    Data_Type<PipelineContext> m_pipelineContext;
  };
}
