# InetAddr 封装与网络地址知识体系

## 1. 三种视图转换 (核心原理)
* **Host (本地)**
    * 格式: string / uint16_t
    * 字节序: Little Endian (小端)
* **Network (传输)**
    * 格式: binary (in_addr_t)
    * 字节序: **Big Endian (大端)** (面试必问)
* **Socket (内核)**
    * 容器: `struct sockaddr_in`
    * API: `bind`, `accept` 只认 `sockaddr*`

## 2. 关键 API (面试考点)
* **字节序转换**
    * `htons()`: Host to Network Short (端口)
    * `ntohs()`: Network to Host Short
* **IP 地址转换**
    * `inet_pton()` / `inet_ntop()`: **推荐**，支持 IPv6，线程安全
    * `inet_addr()` / `inet_ntoa()`: **不推荐**，不支持 IPv6，ntoa 线程不安全(使用静态区)
    * `INADDR_ANY`: 0.0.0.0，绑定本机所有网卡

## 3. 类设计场景
* **Server Listen**: `(port)` -> `INADDR_ANY` + `htons(port)`
* **Server Accept**: `(sockaddr_in)` -> `ntohs` + `inet_ntop`
* **Client Connect**: `(ip, port)` -> `inet_pton` + `htons` 
## 4. 易错点
* **类型强转**: `sockaddr_in*` 必须强转为 `sockaddr*` 传给系统调用
* **内存清零**: 初始化 `sockaddr_in` 前最好 `memset` 或 `bzero`，防止脏数据