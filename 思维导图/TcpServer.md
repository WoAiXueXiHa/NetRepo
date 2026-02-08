# TcpServer 核心架构与实现复盘

## 1. 核心实现思路 (Implementation Logic)
### 1.1 角色定位
- **身份**: "大堂经理" (Connection Dispatcher)
- **职责**: 
    - 只负责 **"接客"** (Accept)
    - 只负责 **"派单"** (Fork)
    - **不负责** "服务" (具体的业务逻辑由回调函数处理)
- **持有资源**: 
    - `_listensock`: 总机电话 (监听套接字)
    - `_handler`: 员工手册 (业务回调函数)

### 1.2 启动阶段 (InitServer)
- **动作 1: 注册回调** (`_handler = handler`)
    - 将上层传来的业务逻辑（如 HTTP 处理）保存下来
    - **意义**: 打通了网络层和业务层的通道
- **动作 2: 激活网络** (`BuildTcpSocketMethod`)
    - 调用 `Socket` 基类的模板方法
    - 一键完成: `Socket()` -> `SetSockOpt()` -> `Bind()` -> `Listen()`

### 1.3 运行阶段 (Loop) - "服务器的心脏"
- **Step 1: 阻塞等待 (Accept)**
    - 从内核全连接队列获取一个 `newsock`
    - **异常处理**: 若 `newsock == nullptr`，直接 `continue` (Fail-Fast)
- **Step 2: 并发调度 (Double Fork "金蝉脱壳"神技)**
    - **爷爷进程 (Server 主进程)**
        - **任务**: 负责生"爸爸"，并回收"爸爸"
        - **资源**: 关闭 `newsock` (爷爷不服务客户，引用计数-1)
        - **回收**: `waitpid()` (因为爸爸瞬间死，所以是非阻塞的，瞬间完成)
    - **爸爸进程 (Middle Man)**
        - **资源**: 关闭 `_listensock` (爸爸不监听)
        - **动作**: `fork()` 生"孙子" -> **立即自杀** (`exit(0)`)
        - **意义**: 唯一的使命就是为了制造"孤儿"
    - **孙子进程 (Worker - 孤儿进程)**
        - **状态**: 爸爸死了，被系统(init/systemd)领养
        - **优势**: 死后由系统自动回收，**彻底杜绝僵尸进程**
        - **任务**: 执行 `_handler(newsock)` (干实事)
        - **结局**: 干完活必须 `exit(0)` (防止跑回 Loop 去 Accept)

## 2. 核心知识点 (Key Knowledge)
### 2.1 多进程编程
- **Fork 原理**: 
    - 复制父进程的所有资源（内存、堆栈、**文件描述符表**）
    - 父子进程共享代码段，但数据独立（写时拷贝）
- **僵尸进程 (Zombie)**: 
    - 子进程退出，父进程没 wait，子进程的尸体(PCB)还留在系统里占用 PID
- **孤儿进程 (Orphan)**: 
    - 父进程先死，子进程被系统接管（系统会自动 wait 它）

### 2.2 智能指针与资源管理 (RAII)
- **工具**: `std::shared_ptr<Socket>`
- **引用计数魔法**: 
    - 爷爷、爸爸、孙子各持有一份 `newsock` 的拷贝（引用计数=3）
    - 爷爷 Close -> 计数=2
    - 爸爸 Exit -> 计数=1
    - 孙子 Exit -> 计数=0 -> **真正调用 `close(fd)`**
- **价值**: 只要对象销毁，连接自动断开，**防止 FD 泄漏**

### 2.3 回调机制 (Callback)
- **定义**: `using tcphandler_t = std::function<void(SockPtr, InetAddr)>`
- **优势**: **解耦**。`TcpServer` 不需要 include `HttpServer`，它可以服务于任何 TCP 协议

### 2.4 指针 vs 整数 (易错点)
- **`SockPtr`**: 是 C++ 对象指针，判空用 `if (ptr == nullptr)`
- **`int fd`**: 是 Linux 文件描述符(整数)，错误判断用 `if (fd < 0)`

## 3. 设计思想 (Design Philosophy)
### 3.1 单一职责原则 (SRP)
- `TcpServer`: 管连接、管并发
- `Handler`: 管业务、管协议
- **互不干扰**: 修改 HTTP 解析逻辑不需要动 TcpServer 的代码

### 3.2 快速失败 (Fail-Fast)
- `Accepter` 失败 -> `continue`
- `Bind` 失败 -> `exit`
- **原则**: 关键步骤出错立即报错或跳过，不带病运行

## 4. 举一反三 (Extrapolation)
### 4.1 如果要换成 多线程 (Multi-thread)?
- **改动**: 把 `fork()` 换成 `pthread_create` 或 `std::thread`
- **注意点**: 
    - **资源共享**: 线程共享内存，`_listensock` 不能在子线程 Close！
    - **锁**: 如果有全局变量（如连接数统计），必须加锁 (`Mutex`)
    - **detach**: 线程创建后要 `detach`，否则主线程要 `join` (会阻塞)

### 4.2 如果要换成 线程池 (ThreadPool)?
- **改动**: 爷爷不再 Fork，而是 `ThreadPool->Push(Task)`
- **流程**: `Accept` -> 封装 Task(newsock) -> 入队 -> 线程池自动取任务
- **优势**: 
    - 避免频繁创建/销毁进程的开销 (CPU 友好)
    - 适合 **高并发 + 短连接** (如 HTTP 1.0)

### 4.3 如果要处理 C10K (1万并发)?
- **改动**: 放弃 `Blocking Accept`，放弃 `Fork/Thread`
- **技术**: **IO 多路复用 (`epoll`)** + **Reactor 模型**
- **逻辑**: 
    - 一个线程监听 10000 个 FD
    - 谁有数据来了，就处理谁
    - **不再阻塞等待某一个特定的连接**