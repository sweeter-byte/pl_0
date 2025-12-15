#include "codegen.h"
#include <iostream>
#include <iomanip>

// ========== SymbolTable 实现 ==========

SymbolTable::SymbolTable() : currentLevel(-1) {
    enterScope();  // 初始化全局作用域
}

void SymbolTable::enterScope() {
    currentLevel++;
    scopes.push_back(std::vector<Symbol>());
    addressStack.push_back(3);  // 每层开始地址为3（0-2为连接数据）
}

void SymbolTable::exitScope() {
    if (currentLevel >= 0) {
        scopes.pop_back();
        addressStack.pop_back();
        currentLevel--;
    }
}

void SymbolTable::addSymbol(const std::string& name, SymbolType type, int value) {
    if (currentLevel >= 0 && static_cast<size_t>(currentLevel) < scopes.size()) {
        int addr = value;
        if (type == SymbolType::VAR) {
            addr = getNextAddress();
        }
        scopes[currentLevel].push_back(Symbol(name, type, currentLevel, addr));
    }
}

Symbol* SymbolTable::lookup(const std::string& name) {
    // 从当前作用域向外查找
    for (int i = currentLevel; i >= 0; i--) {
        for (auto& sym : scopes[i]) {
            if (sym.name == name) {
                return &sym;
            }
        }
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrent(const std::string& name) {
    // 只在当前作用域查找
    if (currentLevel >= 0 && static_cast<size_t>(currentLevel) < scopes.size()) {
        for (auto& sym : scopes[currentLevel]) {
            if (sym.name == name) {
                return &sym;
            }
        }
    }
    return nullptr;
}

int SymbolTable::getNextAddress() {
    if (currentLevel >= 0 && static_cast<size_t>(currentLevel) < addressStack.size()) {
        return addressStack[currentLevel]++;
    }
    return 0;
}

int SymbolTable::getCurrentAddress() const {
    if (currentLevel >= 0 && static_cast<size_t>(currentLevel) < addressStack.size()) {
        return addressStack[currentLevel];
    }
    return 0;
}

void SymbolTable::setAddress(int addr) {
    if (currentLevel >= 0 && static_cast<size_t>(currentLevel) < addressStack.size()) {
        addressStack[currentLevel] = addr;
    }
}

void SymbolTable::printSymbolTable() const {
    std::cout << "\n========== Symbol Table ==========\n";
    std::cout << std::left 
              << std::setw(15) << "Name"
              << std::setw(12) << "Type"
              << std::setw(8) << "Level"
              << "Address/Value\n";
    std::cout << std::string(45, '-') << "\n";
    
    for (int i = 0; i <= currentLevel && static_cast<size_t>(i) < scopes.size(); i++) {
        for (const auto& sym : scopes[i]) {
            std::cout << std::left
                      << std::setw(15) << sym.name
                      << std::setw(12);
            
            switch (sym.type) {
                case SymbolType::CONST:
                    std::cout << "CONST";
                    break;
                case SymbolType::VAR:
                    std::cout << "VAR";
                    break;
                case SymbolType::PROCEDURE:
                    std::cout << "PROCEDURE";
                    break;
            }
            
            std::cout << std::setw(8) << sym.level
                      << sym.address << "\n";
        }
    }
    std::cout << std::string(45, '=') << "\n\n";
}

// ========== CodeGenerator 实现 ==========

CodeGenerator::CodeGenerator() : nextCodeAddress(0) {}

int CodeGenerator::emit(OpCode op, int level, int address) {
    code.push_back(Instruction(op, level, address));
    return nextCodeAddress++;
}

void CodeGenerator::backpatch(int codeIndex, int address) {
    if (codeIndex >= 0 && static_cast<size_t>(codeIndex) < code.size()) {
        code[codeIndex].address = address;
    }
}

std::string CodeGenerator::opCodeToString(OpCode op) {
    switch (op) {
        case OpCode::LIT: return "LIT";
        case OpCode::OPR: return "OPR";
        case OpCode::LOD: return "LOD";
        case OpCode::STO: return "STO";
        case OpCode::CAL: return "CAL";
        case OpCode::INT: return "INT";
        case OpCode::JMP: return "JMP";
        case OpCode::JPC: return "JPC";
        case OpCode::RED: return "RED";
        case OpCode::WRT: return "WRT";
        default: return "UNKNOWN";
    }
}

std::string CodeGenerator::oprTypeToString(int oprType) {
    switch (oprType) {
        case 0: return "RET";
        case 1: return "NEG";
        case 2: return "ADD";
        case 3: return "SUB";
        case 4: return "MUL";
        case 5: return "DIV";
        case 6: return "ODD";
        case 8: return "EQ";
        case 9: return "NEQ";
        case 10: return "LT";
        case 11: return "GEQ";
        case 12: return "GT";
        case 13: return "LEQ";
        default: return "UNKNOWN";
    }
}

void CodeGenerator::printCode() const {
    std::cout << "\n========== Generated Code ==========\n";
    std::cout << std::left 
              << std::setw(8) << "Addr"
              << std::setw(8) << "OP"
              << std::setw(8) << "L"
              << std::setw(8) << "A"
              << "Comment\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (size_t i = 0; i < code.size(); i++) {
        const auto& inst = code[i];
        std::cout << std::left
                  << std::setw(8) << i
                  << std::setw(8) << opCodeToString(inst.op)
                  << std::setw(8) << inst.level
                  << std::setw(8) << inst.address;
        
        // 添加注释
        if (inst.op == OpCode::OPR) {
            std::cout << "; " << oprTypeToString(inst.address);
        } else if (inst.op == OpCode::LIT) {
            std::cout << "; load constant " << inst.address;
        } else if (inst.op == OpCode::JMP) {
            std::cout << "; jump to " << inst.address;
        } else if (inst.op == OpCode::JPC) {
            std::cout << "; jump to " << inst.address << " if false";
        }
        
        std::cout << "\n";
    }
    std::cout << std::string(60, '=') << "\n\n";
}
