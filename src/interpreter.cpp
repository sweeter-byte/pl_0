#include "interpreter.h"
#include <iostream>
#include <iomanip>
#include <cmath>

Interpreter::Interpreter(bool debugMode) 
    : code(), stack(), I(OpCode::LIT, 0, 0), P(0), T(-1), B(0), 
      debug(debugMode), stepCount(0), running(false) {
    stack.resize(STACK_SIZE, 0);
    code.reserve(CODE_SIZE);
}

void Interpreter::loadCode(const std::vector<Instruction>& program) {
    code = program;
    P = 0;
    T = -1;
    B = 0;
    stepCount = 0;
    running = false;
}

// 计算层差为level的活动记录基地址
int Interpreter::base(int level) {
    int base = B;
    while (level > 0) {
        base = stack[base + SL_OFFSET];  // 沿静态链向上查找
        level--;
    }
    return base;
}

void Interpreter::printStack() {
    std::cout << "Stack (T=" << T << ", B=" << B << "): [";
    for (int i = 0; i <= T && i < 20; i++) {  // 只打印前20个元素
        std::cout << stack[i];
        if (i < T && i < 19) std::cout << ", ";
    }
    if (T >= 20) std::cout << ", ...";
    std::cout << "]\n";
}

void Interpreter::printRegisters() {
    std::cout << "Registers: P=" << P 
              << ", T=" << T 
              << ", B=" << B << "\n";
}

// LIT 0, a: 将常量a加载到栈顶
void Interpreter::executeLIT() {
    T++;
    stack[T] = I.address;
    if (debug) {
        std::cout << "  LIT: Push constant " << I.address << " to stack\n";
    }
}

// OPR 0, a: 执行运算
void Interpreter::executeOPR() {
    switch (I.address) {
        case 0:  // RET: 返回
            if (debug) std::cout << "  OPR RET: Return from procedure\n";
            T = B - 1;           // 恢复栈顶
            P = stack[B + RA_OFFSET];  // 恢复程序计数器
            B = stack[B + DL_OFFSET];  // 恢复基地址
            break;
            
        case 1:  // NEG: 取负
            stack[T] = -stack[T];
            if (debug) std::cout << "  OPR NEG: " << stack[T] << "\n";
            break;
            
        case 2:  // ADD: 加法
            T--;
            stack[T] += stack[T + 1];
            if (debug) std::cout << "  OPR ADD: " << stack[T] << "\n";
            break;
            
        case 3:  // SUB: 减法
            T--;
            stack[T] -= stack[T + 1];
            if (debug) std::cout << "  OPR SUB: " << stack[T] << "\n";
            break;
            
        case 4:  // MUL: 乘法
            T--;
            stack[T] *= stack[T + 1];
            if (debug) std::cout << "  OPR MUL: " << stack[T] << "\n";
            break;
            
        case 5:  // DIV: 除法
            T--;
            if (stack[T + 1] != 0) {
                stack[T] /= stack[T + 1];
            } else {
                std::cerr << "Runtime Error: Division by zero\n";
                running = false;
            }
            if (debug) std::cout << "  OPR DIV: " << stack[T] << "\n";
            break;
            
        case 6:  // ODD: 奇数判断
            stack[T] = (stack[T] % 2 == 1) ? 1 : 0;
            if (debug) std::cout << "  OPR ODD: " << stack[T] << "\n";
            break;
            
        case 8:  // EQ: 等于
            T--;
            stack[T] = (stack[T] == stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR EQ: " << stack[T] << "\n";
            break;
            
        case 9:  // NEQ: 不等于
            T--;
            stack[T] = (stack[T] != stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR NEQ: " << stack[T] << "\n";
            break;
            
        case 10: // LT: 小于
            T--;
            stack[T] = (stack[T] < stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR LT: " << stack[T] << "\n";
            break;
            
        case 11: // GEQ: 大于等于
            T--;
            stack[T] = (stack[T] >= stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR GEQ: " << stack[T] << "\n";
            break;
            
        case 12: // GT: 大于
            T--;
            stack[T] = (stack[T] > stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR GT: " << stack[T] << "\n";
            break;
            
        case 13: // LEQ: 小于等于
            T--;
            stack[T] = (stack[T] <= stack[T + 1]) ? 1 : 0;
            if (debug) std::cout << "  OPR LEQ: " << stack[T] << "\n";
            break;
            
        default:
            std::cerr << "Unknown OPR operation: " << I.address << "\n";
            running = false;
    }
}

// LOD L, a: 加载变量到栈顶
void Interpreter::executeLOD() {
    int baseAddr = base(I.level);
    T++;
    stack[T] = stack[baseAddr + I.address];
    if (debug) {
        std::cout << "  LOD: Load from [" << baseAddr << "+" << I.address 
                  << "] = " << stack[T] << "\n";
    }
}

// STO L, a: 将栈顶值存入变量
void Interpreter::executeSTO() {
    int baseAddr = base(I.level);
    stack[baseAddr + I.address] = stack[T];
    if (debug) {
        std::cout << "  STO: Store " << stack[T] << " to [" 
                  << baseAddr << "+" << I.address << "]\n";
    }
    T--;
}

// CAL L, a: 调用过程
void Interpreter::executeCAL() {
    if (debug) {
        std::cout << "  CAL: Call procedure at " << I.address 
                  << " (level diff=" << I.level << ")\n";
    }
    
    // 建立新的活动记录
    // 按照活动记录布局：RA在B+0, DL在B+1, SL在B+2
    stack[T + 1] = P;               // RA: 返回地址
    stack[T + 2] = B;               // DL: 动态链
    stack[T + 3] = base(I.level);   // SL: 静态链
    
    B = T + 1;   // 新的基地址
    P = I.address;  // 跳转到过程入口
}

// INT 0, a: 分配数据空间
void Interpreter::executeINT() {
    T += I.address;
    if (debug) {
        std::cout << "  INT: Allocate " << I.address << " units, T=" << T << "\n";
    }
    if (T >= STACK_SIZE) {
        std::cerr << "Runtime Error: Stack overflow\n";
        running = false;
    }
}

// JMP 0, a: 无条件跳转
void Interpreter::executeJMP() {
    P = I.address;
    if (debug) {
        std::cout << "  JMP: Jump to " << I.address << "\n";
    }
}

// JPC 0, a: 条件跳转（栈顶为0时跳转）
void Interpreter::executeJPC() {
    if (stack[T] == 0) {
        P = I.address;
        if (debug) {
            std::cout << "  JPC: Condition false, jump to " << I.address << "\n";
        }
    } else {
        if (debug) {
            std::cout << "  JPC: Condition true, continue\n";
        }
    }
    T--;
}

// RED L, a: 读入数据
void Interpreter::executeRED() {
    int baseAddr = base(I.level);
    std::cout << "? ";
    std::cin >> stack[baseAddr + I.address];
    if (debug) {
        std::cout << "  RED: Read " << stack[baseAddr + I.address] 
                  << " to [" << baseAddr << "+" << I.address << "]\n";
    }
}

// WRT 0, 0: 输出栈顶值
void Interpreter::executeWRT() {
    std::cout << stack[T] << "\n";
    if (debug) {
        std::cout << "  WRT: Write " << stack[T] << "\n";
    }
    T--;
}

void Interpreter::step() {
    if (static_cast<size_t>(P) >= code.size()) {
        running = false;
        return;
    }
    
    // 取指令
    I = code[P];
    P++;
    stepCount++;
    
    if (debug) {
        std::cout << "\nStep " << stepCount << ": " 
                  << CodeGenerator::opCodeToString(I.op) 
                  << " " << I.level << " " << I.address << "\n";
    }
    
    // 执行指令
    switch (I.op) {
        case OpCode::LIT:
            executeLIT();
            break;
        case OpCode::OPR:
            executeOPR();
            break;
        case OpCode::LOD:
            executeLOD();
            break;
        case OpCode::STO:
            executeSTO();
            break;
        case OpCode::CAL:
            executeCAL();
            break;
        case OpCode::INT:
            executeINT();
            break;
        case OpCode::JMP:
            executeJMP();
            break;
        case OpCode::JPC:
            executeJPC();
            break;
        case OpCode::RED:
            executeRED();
            break;
        case OpCode::WRT:
            executeWRT();
            break;
        default:
            std::cerr << "Unknown instruction: " 
                      << static_cast<int>(I.op) << "\n";
            running = false;
    }
    
    if (debug) {
        printStack();
    }
}

void Interpreter::run() {
    std::cout << "\n========== Program Execution ==========\n\n";
    
    running = true;
    P = 0;
    T = -1;
    B = 0;
    stepCount = 0;
    
    // 执行程序
    while (running && static_cast<size_t>(P) < code.size()) {
        step();
        
        // 检测程序结束：执行了OPR 0 (RET) 且栈已清空（返回到初始状态）
        // 只有当从主程序返回时（T < 0 表示栈已清空）才终止
        if (I.op == OpCode::OPR && I.address == 0 && T < 0) {
            running = false;
        }
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Program terminated.\n";
    std::cout << "Total steps executed: " << stepCount << "\n";
    std::cout << "========================================\n\n";
}

