# snova-asio
[![.github/workflows/ci.yaml](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml/badge.svg)](https://github.com/yinqiwen/snova-asio/actions/workflows/ci.yaml) ![GitHub last commit](https://img.shields.io/github/last-commit/yinqiwen/snova-asio) ![Lines of code](https://img.shields.io/tokei/lines/github/yinqiwen/snova-asio) ![GitHub top language](https://img.shields.io/github/languages/top/yinqiwen/snova-asio) ![GitHub](https://img.shields.io/github/license/yinqiwen/snova-asio)   
A lightweight private proxy tool write by c++20 for low-end boxes or embedded devices.

## Usage

### Remote Server(Exit Node)
```bash
./snova --exit 1 --listen :48100  --server_cipher_key my_test_cipher_key
```

### Local Server(Entry Node)
```bash
./snova --entry 1 --listen :48100  --client_cipher_key my_test_cipher_key --remote <remote_ip>:<remote_port>
```
Now you can config the local server 127.0.0.1:48100 as the socks5 proxy for your apps.    

This tool can also run in a router with redirect mode, eg:  
```bash
./snova --entry 1 --redirect 1 --listen 192.168.1.1:48100  --client_cipher_key my_test_cipher_key --remote <remote_ip>:<remote_port>
```
Now you can config `192.168.1.1:48100` as the iptables redirect target.

### Middle Node(TODO)



