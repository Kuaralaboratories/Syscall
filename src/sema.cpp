#include <cassert>
#include <map>
#include <set>

#include "cfg.h"
#include "sema.h"
#include "utils.h"

namespace syscall {

bool Sema::runFlowSensitiveChecks(const ResolvedFunctionDecl &fn) {
    CFG cfg = CFGBuilder().build(fn);

    bool error = false;
    error |= checkReturnOnAllPaths(fn, cfg);
    error |= checkVariableInitialization(cfg);

    return error;
}

bool Sema::checkReturnOnAllPaths(const ResolvedFunctionDecl &fn, const CFG &cfg) {
    if (fn.type.kind == Type::Kind::Void)
        return false;

    int returnCount = 0;
    bool exitReached = false;

    std::set<int> visited;
    std::vector<int> worklist;
    worklist.emplace_back(cfg.entry);

    while (!worklist.empty()) {
        int bb = worklist.back();
        worklist.pop_back();

        if (!visited.emplace(bb).second)
            continue;

        exitReached |= bb == cfg.exit;

        const auto &[preds, succs, stmts] = cfg.basicBlocks[bb];

        if (!stmts.empty() && dynamic_cast<const ResolvedReturnStmt *>(stmts[0])) {
            ++returnCount;
            continue;
        }

        for (auto &&[succ, reachable] : succs)
            if (reachable)
                worklist.emplace_back(succ);
    }

    if (exitReached || returnCount == 0) {
        report(fn.location,
               returnCount > 0
                   ? "non-void function doesn't return a value on every path"
                   : "non-void function doesn't return a value");
    }

    return exitReached || returnCount == 0;
}

bool Sema::checkVariableInitialization(const CFG &cfg) {
    enum class State { Bottom, Unassigned, Assigned, Top };

    using Lattice = std::map<const ResolvedVarDecl *, State>;

    auto joinStates = [](State s1, State s2) {
        if (s1 == s2)
            return s1;

        if (s1 == State::Bottom)
            return s2;

        if (s2 == State::Bottom)
            return s1;

        return State::Top;
    };

    std::vector<Lattice> curLattices(cfg.basicBlocks.size());
    std::vector<std::pair<SourceLocation, std::string>> pendingErrors;

    bool changed = true;
    while (changed) {
        changed = false;
        pendingErrors.clear();

        for (int bb = cfg.entry; bb != cfg.exit; --bb) {
            const auto &[preds, succs, stmts] = cfg.basicBlocks[bb];

            Lattice tmp;
            for (auto &&pred : preds)
                for (auto &&[decl, state] : curLattices[pred.first])
                    tmp[decl] = joinStates(tmp[decl], state);

            for (auto it = stmts.rbegin(); it != stmts.rend(); ++it) {
                const ResolvedStmt *stmt = *it;

                if (auto *decl = dynamic_cast<const ResolvedDeclStmt *>(stmt)) {
                    tmp[decl->varDecl.get()] =
                        decl->varDecl->initializer ? State::Assigned : State::Unassigned;
                    continue;
                }

                if (auto *assignment = dynamic_cast<const ResolvedAssignment *>(stmt)) {
                    const auto *var =
                        dynamic_cast<const ResolvedVarDecl *>(assignment->variable->decl);

                    assert(var &&
                           "assignment to non-variables should have been caught by sema");

                    if (!var->isMutable && tmp[var] != State::Unassigned) {
                        std::string msg = '\'' + var->identifier + "' cannot be mutated";
                        pendingErrors.emplace_back(assignment->location, std::move(msg));
                    }

                    tmp[var] = State::Assigned;
                    continue;
                }

                if (const auto *dre = dynamic_cast<const ResolvedDeclRefExpr *>(stmt)) {
                    const auto *var = dynamic_cast<const ResolvedVarDecl *>(dre->decl);

                    if (var && tmp[var] != State::Assigned) {
                        std::string msg = '\'' + var->identifier + "' is not initialized";
                        pendingErrors.emplace_back(dre->location, std::move(msg));
                    }

                    continue;
                }
            }

            if (curLattices[bb] != tmp) {
                curLattices[bb] = tmp;
                changed = true;
            }
        }
    }

    for (auto &&[loc, msg] : pendingErrors)
        report(loc, msg);

    return !pendingErrors.empty();
}

bool Sema::insertDeclToCurrentScope(ResolvedDecl &decl) {
    const auto &[foundDecl, scopeIdx] = lookupDecl(decl.identifier);

    if (foundDecl && scopeIdx == 0) {
        report(decl.location, "redeclaration of '" + decl.identifier + '\'');
        return false;
    }

    scopes.back().emplace_back(&decl);
    return true;
}

std::pair<ResolvedDecl *, int> Sema::lookupDecl(const std::string id) {
    int scopeIdx = 0;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        for (auto &&decl : *it) {
            if (decl->identifier != id)
                continue;

            return {decl, scopeIdx};
        }

        ++scopeIdx;
    }

    return {nullptr, -1};
}

std::unique_ptr<ResolvedFunctionDecl> Sema::createBuiltinPrintln() {
    SourceLocation loc{"<builtin>", 0, 0};

    auto param = std::make_unique<ResolvedParamDecl>(loc, "n", Type::builtinNumber());

    std::vector<std::unique_ptr<ResolvedParamDecl>> params;
    params.emplace_back(std::move(param));

    auto block = std::make_unique<ResolvedBlock>(
        loc, std::vector<std::unique_ptr<ResolvedStmt>>());

    return std::make_unique<ResolvedFunctionDecl>(
        loc, "println", Type::builtinVoid(), std::move(params), std::move(block));
}

std::optional<Type> Sema::resolveType(Type parsedType) {
    if (parsedType.kind == Type::Kind::Custom)
        return std::nullopt;

    return parsedType;
}

std::unique_ptr<ResolvedUnaryOperator> Sema::resolveUnaryOperator(const UnaryOperator &unary) {
    varOrReturn(resolvedRHS, resolveExpr(*unary.operand));

    if (resolvedRHS->type.kind == Type::Kind::Void)
        return report(
            resolvedRHS->location,
            "void expression cannot be used as an operand to unary operator");

    return std::make_unique<ResolvedUnaryOperator>(unary.location, unary.op, std::move(resolvedRHS));
}

std::unique_ptr<ResolvedBinaryOperator> Sema::resolveBinaryOperator(const BinaryOperator &binop) {
    varOrReturn(resolvedLHS, resolveExpr(*binop.lhs));
    varOrReturn(resolvedRHS, resolveExpr(*binop.rhs));

    if (resolvedLHS->type.kind == Type::Kind::Void)
        return report(
            resolvedLHS->location,
            "void expression cannot be used as LHS operand to binary operator");

    if (resolvedRHS->type.kind == Type::Kind::Void)
        return report(
            resolvedRHS->location,
            "void expression cannot be used as RHS operand to binary operator");

    assert(resolvedLHS->type.kind == resolvedRHS->type.kind &&
           resolvedLHS->type.kind == Type::Kind::Number &&
           "unexpected type in binop");

    return std::make_unique<ResolvedBinaryOperator>(
        binop.location, binop.op, std::move(resolvedLHS), std::move(resolvedRHS));
}

std::unique_ptr<ResolvedGroupingExpr> Sema::resolveGroupingExpr(const GroupingExpr &grouping) {
    varOrReturn(resolvedExpr, resolveExpr(*grouping.expr));
    return std::make_unique<ResolvedGroupingExpr>(grouping.location, std::move(resolvedExpr));
}

std::unique_ptr<ResolvedDeclRefExpr> Sema::resolveDeclRefExpr(const DeclRefExpr &declRefExpr, bool isCallee) {
    ResolvedDecl *decl = lookupDecl(declRefExpr.identifier).first;
    if (!decl)
        return report(declRefExpr.location,
                      "symbol '" + declRefExpr.identifier + "' not found");

    if (!isCallee && dynamic_cast<ResolvedFunctionDecl *>(decl))
        return report(declRefExpr.location,
                      "expected to call function '" + declRefExpr.identifier + "'");

    return std::make_unique<ResolvedDeclRefExpr>(declRefExpr.location, *decl);
}

std::unique_ptr<ResolvedCallExpr> Sema::resolveCallExpr(const CallExpr &call) {
    const auto *dre = dynamic_cast<const DeclRefExpr *>(call.callee.get());
    if (!dre)
        return report(call.location, "expression cannot be called as a function");

    varOrReturn(resolvedCallee, resolveDeclRefExpr(*dre, true));

    const auto *resolvedFunctionDecl =
        dynamic_cast<const ResolvedFunctionDecl *>(resolvedCallee->decl);

    if (!resolvedFunctionDecl)
        return report(call.location, "calling non-function symbol");

    if (call.arguments.size() != resolvedFunctionDecl->params.size())
        return report(call.location, "argument count mismatch");

    std::vector<std::unique_ptr<ResolvedExpr>> resolvedArgs;
    resolvedArgs.reserve(call.arguments.size());

    for (auto &&arg : call.arguments) {
        varOrReturn(resolvedArg, resolveExpr(*arg));
        resolvedArgs.emplace_back(std::move(resolvedArg));
    }

    return std::make_unique<ResolvedCallExpr>(call.location, std::move(resolvedCallee), std::move(resolvedArgs));
}

std::unique_ptr<ResolvedAssignment> Sema::resolveAssignment(const Assignment &assignment) {
    auto varExpr = dynamic_cast<DeclRefExpr *>(assignment.variable.get());
    if (!varExpr)
        return report(assignment.location, "assignment to non-variable");

    varOrReturn(resolvedVar, resolveDeclRefExpr(*varExpr, false));
    varOrReturn(resolvedValue, resolveExpr(*assignment.value));

    if (resolvedVar->decl->isMutable &&
        resolvedValue->type.kind != resolvedVar->decl->type.kind)
        return report(assignment.location, "incompatible types in assignment");

    return std::make_unique<ResolvedAssignment>(assignment.location, std::move(resolvedVar), std::move(resolvedValue));
}

std::unique_ptr<ResolvedReturnStmt> Sema::resolveReturnStmt(const ReturnStmt &returnStmt) {
    varOrReturn(resolvedValue, resolveExpr(*returnStmt.value));

    if (resolvedValue->type.kind == Type::Kind::Void)
        return report(returnStmt.location, "void expression cannot be returned");

    return std::make_unique<ResolvedReturnStmt>(returnStmt.location, std::move(resolvedValue));
}

std::unique_ptr<ResolvedDeclStmt> Sema::resolveDeclStmt(const DeclStmt &declStmt) {
    if (!resolveType(declStmt.type))
        return report(declStmt.location, "invalid type");

    if (declStmt.initializer) {
        varOrReturn(resolvedInitializer, resolveExpr(*declStmt.initializer));
        if (resolvedInitializer->type.kind != declStmt.type.kind)
            return report(declStmt.location, "incompatible types in initializer");
    }

    auto decl = std::make_unique<ResolvedVarDecl>(declStmt.location, declStmt.identifier, declStmt.type,
                                                   declStmt.initializer ? true : false);

    if (!insertDeclToCurrentScope(*decl))
        return nullptr;

    return std::make_unique<ResolvedDeclStmt>(declStmt.location, std::move(decl));
}

std::unique_ptr<ResolvedExpr> Sema::resolveExpr(const Expr &expr) {
    if (const auto *unary = dynamic_cast<const UnaryOperator *>(&expr))
        return resolveUnaryOperator(*unary);

    if (const auto *binop = dynamic_cast<const BinaryOperator *>(&expr))
        return resolveBinaryOperator(*binop);

    if (const auto *grouping = dynamic_cast<const GroupingExpr *>(&expr))
        return resolveGroupingExpr(*grouping);

    if (const auto *declRef = dynamic_cast<const DeclRefExpr *>(&expr))
        return resolveDeclRefExpr(*declRef, false);

    if (const auto *call = dynamic_cast<const CallExpr *>(&expr))
        return resolveCallExpr(*call);

    if (const auto *assign = dynamic_cast<const Assignment *>(&expr))
        return resolveAssignment(*assign);

    assert(false && "unexpected expression type");
    return nullptr;
}

void Sema::report(SourceLocation location, std::string message) {
    std::cerr << "Error at " << location.file << ":" << location.line << ": " << message << std::endl;
}

} // namespace syscall