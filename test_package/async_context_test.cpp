//
// Copyright Copyright 2009-2024, AMT – The Association For Manufacturing Technology (“AMT”)
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

// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <boost/asio/post.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "mtconnect/configuration/async_context.hpp"

using namespace std;
using namespace mtconnect;
using namespace mtconnect::configuration;

// main
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class AsyncContextTest : public testing::Test
{
};

TEST_F(AsyncContextTest, a_thrown_handler_is_fatal_and_stops_the_context)
{
  // An unhandled exception in a completion handler is fatal: the worker logs it,
  // stops the context, and start() returns a non-zero exit code. The service
  // layer propagates that code so the supervisor (systemd Restart=always)
  // restarts the agent rather than leaving it up-but-not-serving.
  AsyncContext ctx;

  boost::asio::post(ctx.get(), [] { throw std::runtime_error("boom"); });

  // start() blocks until the context stops, so drive it on a separate thread.
  std::atomic<bool> finished {false};
  int exitCode = 0;
  std::thread t([&] {
    exitCode = ctx.start();
    finished = true;
  });

  // Poll up to ~2s so the test cannot hang if the fatal-stop path regresses.
  for (int i = 0; i < 200 && !finished; ++i)
    std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(finished);  // the throwing handler stopped the context
  if (!finished)
    ctx.stop();  // rescue so join() cannot block forever
  t.join();

  EXPECT_EQ(1, exitCode);
  EXPECT_FALSE(ctx.isRunning());
}
