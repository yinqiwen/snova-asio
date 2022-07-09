# snova-asio
[![CIBuild](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml/badge.svg?branch=dev)](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml) [![CodeQL](https://github.com/yinqiwen/snova-asio/actions/workflows/codeql.yaml/badge.svg)](https://github.com/yinqiwen/snova-asio/actions/workflows/codeql.yaml) [![Lint Code Base](https://github.com/yinqiwen/snova-asio/actions/workflows/super-linter.yaml/badge.svg)](https://github.com/yinqiwen/snova-asio/actions/workflows/super-linter.yaml) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/c71b81fecb5e479da6489406bc32894d)](https://www.codacy.com/gh/yinqiwen/snova-asio/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=yinqiwen/snova-asio&amp;utm_campaign=Badge_Grade) ![GitHub last commit](https://img.shields.io/github/last-commit/yinqiwen/snova-asio) ![Lines of code](https://img.shields.io/tokei/lines/github/yinqiwen/snova-asio) ![GitHub top language](https://img.shields.io/github/languages/top/yinqiwen/snova-asio) ![GitHub](https://img.shields.io/github/license/yinqiwen/snova-asio?color=brightgreen)   
A lightweight network proxy tool write by c++20 for low-end boxes or embedded devices.

## Features
- Forward Proxy
- Reverse Proxy

## Usage
Full command options: 
```bash
[snova-asio]# ./bazel-bin/snova/app/snova -h
SNOVA:A private proxy tool for low-end boxes.
Usage: ./bazel-bin/snova/app/snova [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --version                   Display program version information and exit
  --config                    Config file path, all cli options can be set into a toml file as the config.
  --log_file TEXT             Log file, default is stdout
  --max_log_file_size INT     Max log file size
  --max_log_file_num INT      Max log file number
  --alsologtostderr BOOLEAN   Also log to stderr
  --listen TEXT ...           Listen address
  --user TEXT                 Auth user name
  --proxy TEXT                The proxy server to connect remote mux server.
  --remote TEXT               Remote server address
  --conn_num_per_server UINT  Remote server connection number per server.
  --conn_expire_secs UINT     Remote server connection expire seconds, default 1800s.
  --conn_max_inactive_secs UINT
                              Close connection if it's inactive 'conn_max_inactive_secs' ago.
  --max_iobuf_pool_size UINT  IOBuf pool max size
  --stream_io_timeout_secs UINT
                              Proxy stream IO timeout secs, default 300s
  --stat_log_period_secs UINT Print stat log every 'stat_log_period_secs', set it to 0 to disable stat log.
  --client_cipher_method TEXT Client cipher method
  --client_cipher_key TEXT    Client cipher key
  --server_cipher_method TEXT Server cipher method
  --server_cipher_key TEXT    Server cipher key
  --entry BOOLEAN             Run as entry node.
  --middle BOOLEAN            Run as middle node.
  --exit BOOLEAN              Run as exit node.
  --redirect BOOLEAN          Run as redirect server for entry node.
  --entry_socket_send_buffer_size UINT
                              Entry server socket send buffer size.
  --entry_socket_recv_buffer_size UINT
                              Entry server socket recv buffer size.
  -L TEXT ...                 Local tunnel options, foramt  <local port>:<remote host>:<remote port>, only works with entry node.
  -R TEXT ...                 Remote tunnel options, foramt  <remote port>:<local host>:<local port>, only works with exit node.
```

### Private Forward Proxy
First, start exit server on remote machine:   
```bash
./snova --exit 1 --listen :48100  --server_cipher_key my_test_cipher_key
```

Second, start entry server on local machine:
```bash
./snova --entry 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <exit_node_ip>:<exit_node_port>
```
This step would launch a socks5 proxy server on port 48100.

This tool can also run in a router with redirect mode, eg:  
```bash
./snova --entry 1 --redirect 1 --listen 0.0.0.0:48100  --client_cipher_key my_test_cipher_key --remote <exit_node_ip>:<exit_node_port>
```
Now you can config local entry server as the iptables redirect target.

### Private Forward Proxy With Middle Server
If you want use server E as the proxy exit server, but server E has no right to listen on a public IP; and there is a server M which has a public IP;  

First, start middle server on server M:   
```bash
./snova --middle 1 --listen :48100  --server_cipher_key my_test_cipher_key
```

Second, start exit server on server E:
```bash
./snova --exit 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <M IP>:48100  --user bob
```

Finally, start entry server on local machine to connect middle server M: 
```bash
./snova --entry 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <M IP>:48100 --user bob
```
This step would launch a socks5 proxy server on port 48100.



### Private Forward Proxy Chains
First, start exit server on remote machine C:   
```bash
./snova --exit 1 --listen :48100  --server_cipher_key my_test_cipher_key
```

Second, start middle server on remote machine B:
```bash
# the remote address can be set to middle server ip:port
./snova --middle 1 --listen :48100 --server_cipher_key my_test_cipher_key --client_cipher_key my_test_cipher_key --remote <exit_node_ip>:<exit_node_port>
```
You can repeate this step if you want more middle servers.

Finally, start entry server on local machine: 
```bash
./snova --entry 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <middle_node_ip>:<middle_node_port>
```
This step would launch a socks5 proxy server on port 48100.


### Reverse Proxy
#### Local Tunnel
This is an example to expose ssh service on server C to server A via server B by local tunnel mode;

First, start exit server on server B:   
```bash
./snova --exit 1 --listen :48100  --server_cipher_key my_test_cipher_key
```
Second, start entry server on server A to connect server B with local tunnel options to ssh service on server C:
```bash
./snova --entry 1 --client_cipher_key my_test_cipher_key --remote <B ip>:48100 -L 48100:<C ip>:22
```

Now u can connect the ssh service on server C from server A's port 48100:
```bash
ssh username@<A ip> -p 48100
```

#### Remote Tunnel
This is an example to expose ssh service on server C to server A via server B by remote tunnel mode;
First, start entry server on server A:   
```bash
./snova --entry 1 --listen mux://:48100 --server_cipher_key my_test_cipher_key 
```
Second, start exit server on server B connect entry server with remote tunnel options to ssh service on server C:
```bash
./snova --exit 1 --client_cipher_key my_test_cipher_key --remote <A ip>:48100 -R 48100:<C ip>:22
```

Now u can connect the ssh service on server C from server A's port 48100:
```bash
ssh username@<A ip> -p 48100
```

#### Local Tunnel With Middle Server

First, start middle server on server M:   
```bash
./snova --middle 1 --listen :48100 --server_cipher_key my_test_cipher_key 
```

Second, start exit server on server B connect middle server:
```bash
./snova --exit 1 --client_cipher_key my_test_cipher_key --remote <M ip>:48100
```

Finally, start entry server on on server A to connect server M with local tunnel options to ssh service on server C:
```bash
./snova --entry 1 --client_cipher_key my_test_cipher_key --remote <M ip>:48100 -L 48100:<C ip>:22
```

Now u can connect the ssh service on server C from server A's port 48100:
```bash
ssh username@<A ip> -p 48100
```



