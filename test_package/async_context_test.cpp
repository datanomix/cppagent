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

TEST_F(AsyncContextTest, worker_survives_a_thrown_handler)
{
  // Fix #3: a handler that throws must not kill the io_context worker. With a
  // single worker thread handlers run FIFO on that one thread; without the fix
  // the throwing handler tears down the worker and the second handler never
  // runs. With the fix the worker catches, resumes run(), and processes it.
  AsyncContext ctx;

  boost::asio::post(ctx.get(), [] { throw std::runtime_error("boom"); });

  std::atomic<bool> secondRan {false};
  boost::asio::post(ctx.get(), [&] { secondRan = true; });

  // start() blocks in its own run loop, so drive it on a separate thread.
  std::thread t([&] { ctx.start(); });

  // Poll up to ~2s so the test cannot hang even if the fix regresses.
  for (int i = 0; i < 200 && !secondRan; ++i)
    std::this_thread::sleep_for(10ms);

  ctx.stop();
  t.join();

  EXPECT_TRUE(secondRan);
}
