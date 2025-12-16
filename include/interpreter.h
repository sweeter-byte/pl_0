#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "codegen.h"
#include <vector>
#include <stack>
#include <iostream>

// Interpreter configuration
const int STACK_SIZE = 10000;   // stack size
const int CODE_SIZE = 1000;     // code size

// Activation record layout
const int RA_OFFSET = 0;  // Return Address
const int DL_OFFSET = 1;  // Dynamic Link - Caller's activation record base address
const int SL_OFFSET = 2;  // Static Link - 直接外层的活动记录首地址

class Interpreter {
private:
    // 存储器
    std::vector<Instruction> code;     
    std::vector<int> stack;            
    
    // 寄存器
    Instruction I;   // 指令寄存器 - 当前执行的指令
    int P;           // 程序地址寄存器 - 下一条指令地址
    int T;           // 栈顶指示器 - 指向栈顶
    int B;           // 基地址寄存器 - 当前过程数据区起始地址
    
    // 调试和统计
    bool debug;      // 是否打印调试信息
    int stepCount;   // 执行步数
    bool running;    // 是否正在运行
    
    // 辅助函数
    int base(int level);  // 计算层差为level的活动记录基地址
    void printStack();    // 打印栈状态
    void printRegisters(); // 打印寄存器状态
    
    // 指令执行
    void executeLIT();    // LIT: 加载常量
    void executeOPR();    // OPR: 执行运算
    void executeLOD();    // LOD: 加载变量
    void executeSTO();    // STO: 存储变量
    void executeCAL();    // CAL: 调用过程
    void executeINT();    // INT: 分配空间
    void executeJMP();    // JMP: 无条件跳转
    void executeJPC();    // JPC: 条件跳转
    void executeRED();    // RED: 读入数据
    void executeWRT();    // WRT: 输出数据
    
public:
    Interpreter(bool debugMode = false);
    
    void loadCode(const std::vector<Instruction>& program);
    void run();
    void step();  // 单步执行
    
    void setDebug(bool enable) { debug = enable; }
    int getStepCount() const { return stepCount; }
};

#endif // INTERPRETER_H
