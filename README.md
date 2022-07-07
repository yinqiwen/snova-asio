# snova-asio
[![CIBuild](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml/badge.svg?branch=dev)](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml) [![CodeQL](https://github.com/yinqiwen/snova-asio/actions/workflows/codeql.yaml/badge.svg)](https://github.com/yinqiwen/snova-asio/actions/workflows/codeql.yaml) [![Lint Code Base](https://github.com/yinqiwen/snova-asio/actions/workflows/super-linter.yaml/badge.svg)](https://github.com/yinqiwen/snova-asio/actions/workflows/super-linter.yaml) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/c71b81fecb5e479da6489406bc32894d)](https://www.codacy.com/gh/yinqiwen/snova-asio/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=yinqiwen/snova-asio&amp;utm_campaign=Badge_Grade) ![GitHub last commit](https://img.shields.io/github/last-commit/yinqiwen/snova-asio) ![Lines of code](https://img.shields.io/tokei/lines/github/yinqiwen/snova-asio) ![GitHub top language](https://img.shields.io/github/languages/top/yinqiwen/snova-asio) ![GitHub](https://img.shields.io/github/license/yinqiwen/snova-asio?color=brightgreen)   
A lightweight network proxy tool write by c++20 for low-end boxes or embedded devices.

## Features
- Forward Proxy
- Reverse Proxy

## Usage

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
If you want use node E as the proxy exit server, but node E has no right to listen on a public IP; and there is a node M which has a public IP;  

First, start middle server on node M:   
```bash
./snova --middle 1 --listen :48100  --client_cipher_key my_test_cipher_key
```

Second, start exit server on node E:
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
./snova --middle 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <exit_node_ip>:<exit_node_port>
```
You can repeate this step if you want more middle servers.

Finally, start entry server on local machine: 
```bash
./snova --entry 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <middle_node_ip>:<middle_node_port>
```
This step would launch a socks5 proxy server on port 48100.


### Reverse Proxy
#### Local Tunnel
This is an example to expose ssh service on node C to node A via node B by local tunnel mode;

First, start exit server on node B:   
```bash
./snova --exit 1 --listen :48100  --server_cipher_key my_test_cipher_key
```
Second, start entry server on node A to connect node B with local tunnel options to ssh service on node C:
```bash
./snova --entry 1 --client_cipher_key my_test_cipher_key --remote <B ip>:48100 -L 48100:<C ip>:22
```

Now u can connect the ssh service on node C from node A's port 48100:
```bash
ssh username@<A ip> -p 48100
```

#### Remote Tunnel
This is an example to expose ssh service on node C to node A via node B by remote tunnel mode;
First, start entry server on node A:   
```bash
./snova --entry 1 --listen mux://:48100 --server_cipher_key my_test_cipher_key 
```
Second, start exit server on node B connect entry node server with remote tunnel options to ssh service on node C:
```bash
./snova --exit 1 --client_cipher_key my_test_cipher_key --remote <A ip>:48100 -R 48100:<C ip>:22
```

Now u can connect the ssh service on node C from node A's port 48100:
```bash
ssh username@<A ip> -p 48100
```

#### Local Tunnel With Middle Server

First, start middle server on node M:   
```bash
./snova --middle 1 --listen :48100 --server_cipher_key my_test_cipher_key 
```

Second, start exit server on node B connect middle node server:
```bash
./snova --exit 1 --client_cipher_key my_test_cipher_key --remote <M ip>:48100
```

Finally, start entry server on on node A to connect node M with local tunnel options to ssh service on node C:
```bash
./snova --entry 1 --client_cipher_key my_test_cipher_key --remote <M ip>:48100 -L 48100:<C ip>:22
```

Now u can connect the ssh service on node C from node A's port 48100:
```bash
ssh username@<A ip> -p 48100
```



