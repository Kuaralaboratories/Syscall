#ifndef SYSCALL_CFG_H
#define SYSCALL_CFG_H

#include <set>
#include <vector>

#include "ast.h"
#include "constexpr.h"

namespace syscall {

// Forward declarations for Syscall-specific types
struct SyscallExpr;
struct SyscallStmt;
struct SyscallFunctionDecl;
struct SyscallBlock;
struct SyscallIfStmt;
struct SyscallWhileStmt;

// Represents a basic block in the control flow graph
struct BasicBlock {
  std::set<std::pair<int, bool>> predecessors; // (blockIndex, isEdgeReachable)
  std::set<std::pair<int, bool>> successors;   // (blockIndex, isEdgeReachable)
  std::vector<const SyscallStmt *> statements; // Statements within the block
};

// Represents the entire control flow graph
struct CFG {
  std::vector<BasicBlock> basicBlocks; // List of basic blocks
  int entry = -1;                      // Entry block index
  int exit = -1;                       // Exit block index

  // Insert a new basic block into the CFG
  int insertNewBlock() {
    basicBlocks.emplace_back();
    return basicBlocks.size() - 1;
  }

  // Insert a new basic block before an existing block
  int insertNewBlockBefore(int before, bool reachable) {
    int b = insertNewBlock();
    insertEdge(b, before, reachable);
    return b;
  }

  // Insert an edge between two blocks
  void insertEdge(int from, int to, bool reachable) {
    basicBlocks[from].successors.emplace(std::make_pair(to, reachable));
    basicBlocks[to].predecessors.emplace(std::make_pair(from, reachable));
  }

  // Insert a statement into a specific block
  void insertStmt(const SyscallStmt *stmt, int block) {
    basicBlocks[block].statements.emplace_back(stmt);
  }

  // Dump the CFG for debugging purposes
  void dump() const;
};

// Builder for generating a CFG from Syscall function declarations
class CFGBuilder {
  ConstantExpressionEvaluator cee; // Expression evaluator
  CFG cfg;                         // Control flow graph being built

  // Insert a block with a given successor
  int insertBlock(const SyscallBlock &block, int successor);

  // Insert an if statement with an exit block
  int insertIfStmt(const SyscallIfStmt &stmt, int exit);

  // Insert a while statement with an exit block
  int insertWhileStmt(const SyscallWhileStmt &stmt, int exit);

  // Insert a statement into a specific block
  int insertStmt(const SyscallStmt &stmt, int block);

  // Insert a declaration statement into a block
  int insertDeclStmt(const SyscallStmt &stmt, int block);

  // Insert an assignment into a block
  int insertAssignment(const SyscallStmt &stmt, int block);

  // Insert a return statement into a block
  int insertReturnStmt(const SyscallStmt &stmt, int block);

  // Insert an expression into a block
  int insertExpr(const SyscallExpr &expr, int block);

public:
  // Build a CFG from a Syscall function declaration
  CFG build(const SyscallFunctionDecl &fn);
};

} // namespace syscall

#endif // SYSCALL_CFG_H
