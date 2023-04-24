//
// Copyright Copyright 2009-2022, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "mtconnect/config.hpp"
#include "mtconnect/utilities.hpp"
#include "shdr_tokenizer.hpp"
#include "transform.hpp"

namespace mtconnect::pipeline {
  using namespace entity;

  /// @brief An entity that carries a timestamp and a token list
  class AGENT_LIB_API Timestamped : public Tokens
  {
  public:
    using Tokens::Tokens;
    Timestamped(const Timestamped &ts) = default;
    Timestamped(const Tokens &ptr) : Tokens(ptr) {}
    Timestamped(const Timestamped &ts, TokenList list)
      : Tokens(ts, list), m_timestamp(ts.m_timestamp), m_duration(ts.m_duration)
    {}
    ~Timestamped() = default;
    Timestamp m_timestamp;
    std::optional<double> m_duration;  ///< Optional duration
  };
  using TimestampedPtr = std::shared_ptr<Timestamped>;

  /// @brief A timestamped asset command
  class AGENT_LIB_API AssetCommand : public Timestamped
  {
  public:
    using Timestamped::Timestamped;
  };

  /// @brief A transform to extract the timestamp
  class AGENT_LIB_API ExtractTimestamp : public Transform
  {
  public:
    ExtractTimestamp(const ExtractTimestamp &) = default;
    /// @brief Construct a timestamp extractor
    /// @param relativeTime `true` if using realtive time stamps
    ExtractTimestamp(bool relativeTime)
      : Transform("ExtractTimestamp"), m_relativeTime(relativeTime)
    {
      m_guard = TypeGuard<Tokens>(RUN);
    }
    ~ExtractTimestamp() override = default;

    using Now = std::function<Timestamp()>;
    EntityPtr operator()(entity::EntityPtr &&ptr) override
    {
      TimestampedPtr res;
      std::optional<std::string> token;
      if (auto tokens = std::dynamic_pointer_cast<Tokens>(ptr);
          tokens && tokens->m_tokens.size() > 0)
      {
        res = std::make_shared<Timestamped>(*tokens);
        token = res->m_tokens.front();
        res->m_tokens.pop_front();
      }
      else if (ptr->hasProperty("timestamp"))
      {
        token = res->maybeGet<std::string>("timestamp");
        if (token)
          res->erase("timestamp");
      }

      if (token)
        extractTimestamp(*token, res);
      else
        res->m_timestamp = now();

      res->setProperty("timestamp", res->m_timestamp);
      return next(res);
    }

    void extractTimestamp(const std::string &token, TimestampedPtr &ts);
    inline Timestamp now() { return m_now ? m_now() : std::chrono::system_clock::now(); }

    Now m_now;

  protected:
    bool m_relativeTime {false};
    std::optional<Timestamp> m_base;
    Microseconds m_offset;
  };

  /// @brief Always use agent time and remove first token
  class AGENT_LIB_API IgnoreTimestamp : public ExtractTimestamp
  {
  public:
    IgnoreTimestamp() : ExtractTimestamp("IgnoreTimestamp") {}
    IgnoreTimestamp(const IgnoreTimestamp &) = default;
    ~IgnoreTimestamp() override = default;

    EntityPtr operator()(entity::EntityPtr &&ptr) override
    {
      TimestampedPtr res;
      std::optional<std::string> token;
      if (auto tokens = std::dynamic_pointer_cast<Tokens>(ptr);
          tokens && tokens->m_tokens.size() > 0)
      {
        res = std::make_shared<Timestamped>(*tokens);
        res->m_tokens.pop_front();
      }
      else if (res->hasProperty("timestamp"))
      {
        res->erase("timestamp");
      }
      res->m_timestamp = now();
      res->setProperty("timestamp", res->m_timestamp);

      return next(res);
    }
  };
}  // namespace mtconnect::pipeline
