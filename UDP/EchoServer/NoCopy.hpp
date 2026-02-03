#pragma once

// 防止对象被意外拷贝导致资源重复释放or混乱
// 定义一个NoCopy基类

// socket是系统资源（文件描述符），如果浅拷贝，
// 两个对象析构时会关闭同一个fd两次，导致程序崩溃
class NoCopy{
public:
    NoCopy(){}
    ~NoCopy(){}

    // 禁止拷贝构造和赋值重载
    NoCopy(const NoCopy&) = delete;
    const NoCopy& operator=(const NoCopy&) = delete;
};