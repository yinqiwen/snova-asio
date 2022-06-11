/*
 *Copyright (c) 2022, qiyingwang <qiyingwang@tencent.com>
 *All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of rimos nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <memory>
#include <string>
#include <vector>

#include "asio.hpp"
#include "snova/mux/mux_connection.h"
namespace snova {
class MuxClient {
 public:
  static std::shared_ptr<MuxClient>& GetInstance();
  static EventWriterFactory GetEventWriterFactory();
  void SetClientId(uint64_t client_id);
  asio::awaitable<std::error_code> Init(const std::string& user, const std::string& cipher_method,
                                        const std::string& cipher_key);
  MuxConnectionPtr SelectConnection();

 private:
  asio::awaitable<std::error_code> NewConnection(uint32_t idx);
  asio::awaitable<void> CheckConnections();

  std::vector<MuxConnectionPtr> remote_conns_;
  ::asio::ip::tcp::endpoint remote_endpoint_;
  std::string auth_user_;
  std::string cipher_method_;
  std::string cipher_key_;
  uint32_t select_cursor_ = 0;
  uint64_t client_id_ = 0;
};
void register_mux_stat();

}  // namespace snova