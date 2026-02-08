# Socket 模块：网络通信基石

## 1. 设计思想 (Architecture)
- **模板方法模式 (Template Method)**
    - **核心逻辑**：基类制定流程 (`BuildTcpSocketMethod`)，子类填空 (`BindOrDie` 等)。
    - **目的**：强制规范启动顺序 (Socket -> SetOpt -> Bind -> Listen)，防止开发者漏写步骤。
- **面向接口编程**
    - **基类 (`Socket`)**：定义纯虚函数接口，充当“老板”角色，只管发号施令。
    - **子类 (`TcpSocket`)**：实现具体系统调用，充当“员工”角色，负责干脏活累活。
- **RAII 资源管理**
    - **智能指针 (`std::shared_ptr`)**：自动管理 `Socket` 对象生命周期。
    - **自动释放**：对象析构时自动调用 `close(fd)`，杜绝文件描述符泄漏 (FD Leak)。
- **Fail-Fast (快速失败)**
    - **原则**：启动阶段遇到错误（如端口被占）立即 `exit`，不留隐患。

## 2. 核心代码实现 (Implementation)
- **基类 `Socket` (Abstract)**
    - **纯虚函数**：`virtual void SocketOrDie() = 0;` (强制子类实现)
    - **模板方法**：`void BuildTcpSocketMethod(int port)` (定死流程)
- **子类 `TcpSocket` (Concrete)**
    - **成员变量**：`int _sockfd` (持有系统分配的文件描述符)
    - **构造函数**：初始化 `_sockfd = -1`，支持从现有 FD 构造 (用于 Accept 返回)。
    - **`Fd()` 实现**：[重要] 必须返回 `_sockfd`，否则无法进行 IO 操作。

## 3. 关键知识点 (Key Points)
- **C++ 特性**
    - **多态 (Polymorphism)**：通过基类指针调用子类方法。
    - **纯虚函数**：接口定义的标准方式。
    - **override**：确保子类正确覆盖基类函数，防止拼写错误。
- **网络编程 API**
    - **`socket`**：创建端点 (AF_INET, SOCK_STREAM)。
    - **`bind`**：绑定 IP 和 Port (关联 `InetAddr`)。
    - **`listen`**：将 Socket 标记为被动监听状态，开启全连接队列。
    - **`accept`**：从队列取出已完成连接，**返回新的 FD** (ConnFD)。
    - **`setsockopt`**：
        - **SO_REUSEADDR**：允许立即重用 TIME_WAIT 状态的端口 (开发必备)。
- **调试技巧**
    - **`errno` & `strerror`**：打印系统调用失败的具体原因。
    - **`netstat`/`lsof`**：查看端口占用情况。

## 4. 举一反三 (Extensibility)
- **场景 A：UDP 服务器**
    - **新建类**：`class UdpSocket : public Socket`
    - **差异**：
        - `SocketOrDie` 使用 `SOCK_DGRAM`。
        - `ListenOrDie` 和 `Accepter` 空实现 (UDP 无连接)。
        - `Recv/Send` 使用 `recvfrom/sendto`。
- **场景 B：Unix Domain Socket**
    - **用途**：本机进程间通信 (比 TCP 快)。
    - **差异**：地址结构体使用 `sockaddr_un`，文件路径代替 IP:Port。

## 5. 验证与测试 (Verification)
- **基本测试**：编写 `main` 函数，调用 `BuildTcpSocketMethod`，死循环 `Accept`。
- **连通性检查**：
    - `telnet 127.0.0.1 8080` (最简单)
    - 浏览器访问 `http://127.0.0.1:8080` (看日志是否有 Accept 记录)
- **常见 Bug**
    - **Bind Error**：端口被占 (检查 `SO_REUSEADDR` 或杀进程)。
    - **Accept 阻塞**：正常现象，直到有客户端连接。
    - **Fd 泄漏**：检查析构函数是否调用 `close`。