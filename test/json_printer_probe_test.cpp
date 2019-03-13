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

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <nlohmann/json.hpp>

#include "component_event.hpp"
#include "data_item.hpp"
#include "device.hpp"
#include "json_printer.hpp"
#include "globals.hpp"
#include "checkpoint.hpp"
#include "xml_parser.hpp"
#include "xml_printer.hpp"
#include "test_globals.hpp"

using json = nlohmann::json;
using namespace std;

namespace mtconnect {
  namespace test {
    constexpr nlohmann::json::size_type operator "" _S(unsigned long long v)
    { 
      return static_cast<nlohmann::json::size_type>(v);
    }

    class JsonPrinterProbeTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(JsonPrinterProbeTest);
      CPPUNIT_TEST(testDeviceRootAndDescription);
      CPPUNIT_TEST(testTopLevelDataItems);
      CPPUNIT_TEST(testSubComponents);
      CPPUNIT_TEST_SUITE_END();
      
    public:
      void testDeviceRootAndDescription();
      void testTopLevelDataItems();
      void testSubComponents();

      void setUp();
      void tearDown();

    protected:
      std::unique_ptr<JsonPrinter> m_printer;
      std::vector<Device *> m_devices;
      
      std::unique_ptr<XmlParser> m_config;
      std::unique_ptr<XmlPrinter> m_xmlPrinter;
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(JsonPrinterProbeTest);

    void JsonPrinterProbeTest::setUp()
    {
      m_xmlPrinter.reset(new XmlPrinter("1.5"));
      m_printer.reset(new JsonPrinter("1.5", true));
      
      m_config.reset(new XmlParser());
      m_devices = m_config->parseFile("../samples/SimpleDevlce.xml", m_xmlPrinter.get());
    }

    void JsonPrinterProbeTest::tearDown()
    {
      m_config.reset();
      m_xmlPrinter.reset();
      m_printer.reset();
      m_devices.clear();
    }
    
    void JsonPrinterProbeTest::testDeviceRootAndDescription()
    {
      auto doc = m_printer->printProbe(123, 9999, 1, 1024, 10, m_devices);
      auto jdoc = json::parse(doc);
      auto it = jdoc.begin();
      CPPUNIT_ASSERT_EQUAL(string("MTConnectDevices"), it.key());
      CPPUNIT_ASSERT_EQUAL(123, jdoc.at("/MTConnectDevices/Header/@instanceId"_json_pointer).get<int32_t>());
      CPPUNIT_ASSERT_EQUAL(9999, jdoc.at("/MTConnectDevices/Header/@bufferSize"_json_pointer).get<int32_t>());
      CPPUNIT_ASSERT_EQUAL(1024, jdoc.at("/MTConnectDevices/Header/@assetBufferSize"_json_pointer).get<int32_t>());
      CPPUNIT_ASSERT_EQUAL(10, jdoc.at("/MTConnectDevices/Header/@assetCount"_json_pointer).get<int32_t>());

      auto devices = jdoc.at("/MTConnectDevices/Devices"_json_pointer);
      auto device = devices.at(0).at("/Device"_json_pointer);

      CPPUNIT_ASSERT_EQUAL(string("x872a3490"), device.at("/@id"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("SimpleCnc"), device.at("/@name"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("872a3490-bd2d-0136-3eb0-0c85909298d9"), device.at("/@uuid"_json_pointer).get<string>());
      
      CPPUNIT_ASSERT_EQUAL(string("This is a simple CNC example"), device.at("/Description/#text"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("MTConnectInstitute"), device.at("/Description/@manufacturer"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("12"), device.at("/Description/@serialNumber"_json_pointer).get<string>());
    }
    
    void JsonPrinterProbeTest::testTopLevelDataItems()
    {
      auto doc = m_printer->printProbe(123, 9999, 1, 1024, 10, m_devices);
      //cout << "\n" << doc << endl;
      auto jdoc = json::parse(doc);
      auto devices = jdoc.at("/MTConnectDevices/Devices"_json_pointer);
      auto device = devices.at(0).at("/Device"_json_pointer);
      
      auto dataItems = device.at("/DataItems"_json_pointer);
      CPPUNIT_ASSERT(dataItems.is_array());
      CPPUNIT_ASSERT_EQUAL(3_S, dataItems.size());
      
      // Alarm event
      auto avail = dataItems.at(0);
      CPPUNIT_ASSERT_EQUAL(string("AVAILABILITY"), avail.at("/DataItem/@type"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("EVENT"), avail.at("/DataItem/@category"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("d5b078a0"), avail.at("/DataItem/@id"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("avail"), avail.at("/DataItem/@name"_json_pointer).get<string>());
      
      // Availability event
      auto change = dataItems.at(1);
      CPPUNIT_ASSERT_EQUAL(string("ASSET_CHANGED"), change.at("/DataItem/@type"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("EVENT"), change.at("/DataItem/@category"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("e4a300e0"), change.at("/DataItem/@id"_json_pointer).get<string>());

      auto remove = dataItems.at(2);
      CPPUNIT_ASSERT_EQUAL(string("ASSET_REMOVED"), remove.at("/DataItem/@type"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("EVENT"), remove.at("/DataItem/@category"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("f2df7550"), remove.at("/DataItem/@id"_json_pointer).get<string>());
    }
    
    void JsonPrinterProbeTest::testSubComponents()
    {
      auto doc = m_printer->printProbe(123, 9999, 1, 1024, 10, m_devices);
      auto jdoc = json::parse(doc);
      auto devices = jdoc.at("/MTConnectDevices/Devices"_json_pointer);
      auto device = devices.at(0).at("/Device"_json_pointer);

      auto components = device.at("/Components"_json_pointer);
      CPPUNIT_ASSERT(components.is_array());
      CPPUNIT_ASSERT_EQUAL(3_S, components.size());

      auto axes = components.at(0);
      CPPUNIT_ASSERT(axes.is_object());
      CPPUNIT_ASSERT_EQUAL(string("Axes"), axes.begin().key());
      CPPUNIT_ASSERT_EQUAL(string("a62a1050"), axes.at("/Axes/@id"_json_pointer).get<string>());

      auto subAxes = axes.at("/Axes/Components"_json_pointer);
      CPPUNIT_ASSERT(subAxes.is_array());
      CPPUNIT_ASSERT_EQUAL(2_S, subAxes.size());
      
      auto rotary = subAxes.at(0);
      CPPUNIT_ASSERT(rotary.is_object());
      auto rc = rotary.at("/Linear"_json_pointer);
      CPPUNIT_ASSERT(rc.is_object());
      CPPUNIT_ASSERT_EQUAL(string("X1"), rc.at("/@name"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("X"), rc.at("/@nativeName"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("e373fec0"), rc.at("/@id"_json_pointer).get<string>());
      
      auto dataItems = rc.at("/DataItems"_json_pointer);
      CPPUNIT_ASSERT(dataItems.is_array());
      
      auto ss = dataItems.at(0).at("/DataItem"_json_pointer);
      CPPUNIT_ASSERT(ss.is_object());
      CPPUNIT_ASSERT_EQUAL(string("POSITION"), ss.at("/@type"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("MILLIMETER"), ss.at("/@units"_json_pointer).get<string>());
      CPPUNIT_ASSERT_EQUAL(string("ACTUAL"), ss.at("/@subType"_json_pointer).get<string>());
    }

  }
}

      