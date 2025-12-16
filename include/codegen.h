#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// 操作码枚举
enum class OpCode {
    LIT,  // 0: 加载常量到栈顶
    OPR,  // 1: 执行运算
    LOD,  // 2: 加载变量到栈顶
    STO,  // 3: 存储栈顶到变量
    CAL,  // 4: 调用过程
    INT,  // 5: 分配数据空间
    JMP,  // 6: 无条件跳转
    JPC,  // 7: 条件跳转
    RED,  // 8: 读入数据
    WRT   // 9: 输出数据
};

// OPR操作的具体类型
enum class OprType {
    RET = 0,    // 返回
    NEG = 1,    // 取负
    ADD = 2,    // 加
    SUB = 3,    // 减
    MUL = 4,    // 乘
    DIV = 5,    // 除
    ODD = 6,    // 奇数判断
    // 7 保留(后面拓展为 mod)
    EQ = 8,     // 等于
    NEQ = 9,    // 不等于
    LT = 10,    // 小于
    GEQ = 11,   // 大于等于
    GT = 12,    // 大于
    LEQ = 13    // 小于等于
};

// 符号类型
enum class SymbolType {
    CONST,      // 常量
    VAR,        // 变量
    PROCEDURE   // 过程
};

// 符号表项
struct Symbol {
    std::string name;
    SymbolType type;
    int level;      // 层次
    int address;    // 地址（对于VAR）或值（对于CONST）或入口地址（对于PROCEDURE）
    
    Symbol(const std::string& n, SymbolType t, int l, int a)
        : name(n), type(t), level(l), address(a) {}
};

// 指令结构
struct Instruction {
    OpCode op;
    int level;      // L段：层差
    int address;    // A段：地址/操作数
    
    Instruction(OpCode o, int l, int a) : op(o), level(l), address(a) {}
};

// 符号表（支持作用域）
class SymbolTable {
private:
    std::vector<std::vector<Symbol>> scopes;  // 每一层作用域的符号, scopes[0] means global variable, scopes[1] means fisrt level
    int currentLevel;
    std::vector<int> addressStack;  // 每层的地址计数
    
public:
    SymbolTable();
    
    void enterScope();
    void exitScope();
    
    void addSymbol(const std::string& name, SymbolType type, int value);
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrent(const std::string& name);  // 只在当前作用域查找
    
    int getCurrentLevel() const { return currentLevel; }
    int getNextAddress();
    int getCurrentAddress() const;
    void setAddress(int addr);
    
    void printSymbolTable() const;
};

// 代码生成器
class CodeGenerator {
private:
    std::vector<Instruction> code;
    int nextCodeAddress;
    
public:
    CodeGenerator();
    
    int emit(OpCode op, int level, int address);
    void backpatch(int codeIndex, int address);
    int getNextAddress() const { return nextCodeAddress; }
    
    const std::vector<Instruction>& getCode() const { return code; }
    void printCode() const;
    
    static std::string opCodeToString(OpCode op);
    static std::string oprTypeToString(int oprType);
};

#endif // CODEGEN_H
