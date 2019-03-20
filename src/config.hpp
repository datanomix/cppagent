//
// Copyright Copyright 2009-2019, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "globals.hpp"

#include "service.hpp"
#include <string>
#include <chrono>
#include "dlib/logger.h"

namespace mtconnect {
  class Agent;
  class Device;
  class log_level;
  class RollingFileLogger;
  class XmlPrinter;
  
  typedef void (XmlPrinter::* NamespaceFunction)(const std::string &urn, const std::string &location, const std::string &prefix);
  typedef void (XmlPrinter::* StyleFunction)(const std::string &location);
  
  class AgentConfiguration : public MTConnectService
  {
  public:
    AgentConfiguration();
    virtual ~AgentConfiguration();
    
    // For MTConnectService
    void stop() override;
    void start() override;
    void initialize(int argc, const char *argv[]) override;
    
    void configureLogger(dlib::config_reader::kernel_1a &reader);
    void loadConfig(std::istream &file);
    
    void setAgent(Agent *agent) {
      m_agent = agent; }
    const Agent * getAgent() const {
      return m_agent; }
    
    const RollingFileLogger *getLogger() const {
      return m_loggerFile; }
    
  protected:
    Device *defaultDevice();
    void loadAdapters(dlib::config_reader::kernel_1a &reader,
                      bool defaultPreserve,
                      std::chrono::seconds legacyTimeout,
                      std::chrono::milliseconds reconnectInterval,
                      bool ignoreTimestamps,
                      bool conversionRequired,
                      bool upcaseValue);
    void loadAllowPut(dlib::config_reader::kernel_1a &reader);
    void loadNamespace(dlib::config_reader::kernel_1a &reader,
                       const char *namespaceType, XmlPrinter *printer,
                       NamespaceFunction callback);
    void loadFiles(dlib::config_reader::kernel_1a &reader);
    void loadStyle(dlib::config_reader::kernel_1a &reader, const char *styleName, XmlPrinter *printer, StyleFunction styleFunction);
    void loadTypes(dlib::config_reader::kernel_1a &reader);
    
    void LoggerHook(const std::string& loggerName,
                    const dlib::log_level& l,
                    const dlib::uint64 threadId,
                    const char* message);
    
    void monitorThread();
    
  protected:
    Agent *m_agent;
    RollingFileLogger *m_loggerFile;
    bool m_monitorFiles;
    int m_minimumConfigReloadAge;
    std::string m_devicesFile;
    bool m_restart;
    std::string m_exePath;
    bool m_pretty;
  };
}
