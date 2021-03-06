/*
 *Copyright (c) 2022, yinqiwen <yinqiwen@gmail.com>
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
#include <system_error>
#include <vector>

#include "asio.hpp"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/io.h"
#include "snova/util/address.h"
namespace snova {
asio::awaitable<std::error_code> start_entry_server(const NetAddress& addr);

asio::awaitable<void> handle_socks5_connection(::asio::ip::tcp::socket&& sock,
                                               IOBufPtr&& read_buffer, const Bytes& readable_data);
asio::awaitable<bool> handle_tls_connection(
    ::asio::ip::tcp::socket&& sock, IOBufPtr&& read_buffer, const Bytes& readable_data,
    std::unique_ptr<::asio::ip::tcp::endpoint>&& orig_remote_endpoint);
asio::awaitable<void> handle_http_connection(::asio::ip::tcp::socket&& sock, IOBufPtr&& read_buffer,
                                             Bytes& readable_data);

}  // namespace snova
