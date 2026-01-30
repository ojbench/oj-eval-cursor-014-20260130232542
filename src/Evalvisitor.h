#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <memory>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Arbitrary precision integer
class BigInt {
public:
    BigInt() : negative(false), digits("0") {}
    BigInt(long long n);
    explicit BigInt(const std::string& s);

    std::string toString() const;
    long long toLong() const;  // for int() conversion when in range
    bool isZero() const { return digits == "0" || digits.empty(); }

    BigInt operator-() const;
    BigInt operator+(const BigInt& o) const;
    BigInt operator-(const BigInt& o) const;
    BigInt operator*(const BigInt& o) const;
    BigInt operator/(const BigInt& o) const;  // floor division
    BigInt operator%(const BigInt& o) const;

    bool operator<(const BigInt& o) const;
    bool operator>(const BigInt& o) const;
    bool operator<=(const BigInt& o) const;
    bool operator>=(const BigInt& o) const;
    bool operator==(const BigInt& o) const;
    bool operator!=(const BigInt& o) const;

    BigInt& operator+=(const BigInt& o);
    BigInt& operator-=(const BigInt& o);
    BigInt& operator*=(const BigInt& o);
    BigInt& operator/=(const BigInt& o);
    BigInt& operator%=(const BigInt& o);

private:
    bool negative;
    std::string digits;  // absolute value, no leading zeros

    static std::string add(const std::string& a, const std::string& b);
    static std::string sub(const std::string& a, const std::string& b);
    static std::string mul(const std::string& a, const std::string& b);
    static std::string divmod(const std::string& a, const std::string& b, std::string& rem);
    static int compare(const std::string& a, const std::string& b);
    static std::string trimZeros(std::string s);
};

// Value type: None, int (BigInt), float, bool, str, tuple (for multiple return/assign)
struct PyNone {};
struct PyTuple;  // forward
using Value = std::variant<PyNone, BigInt, double, bool, std::string, std::shared_ptr<PyTuple>>;
struct PyTuple { std::vector<Value> elts; };

enum class FlowType { Normal, Return, Break, Continue };

class EvalVisitor : public Python3ParserBaseVisitor {
public:
    std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    std::any visitParameters(Python3Parser::ParametersContext *ctx) override;
    std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    std::any visitAugassign(Python3Parser::AugassignContext *ctx) override;
    std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    std::any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override;
    std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    std::any visitTest(Python3Parser::TestContext *ctx) override;
    std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    std::any visitTerm(Python3Parser::TermContext *ctx) override;
    std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    std::any visitTrailer(Python3Parser::TrailerContext *ctx) override;
    std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    std::any visitFormat_string(Python3Parser::Format_stringContext *ctx) override;
    std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;

private:
    std::vector<std::map<std::string, Value>> scopes;
    std::map<std::string, std::tuple<std::vector<std::string>, std::vector<Value>, Python3Parser::SuiteContext*>> functions;
    Python3Parser::Atom_exprContext* currentAtomExpr = nullptr;  // for trailer to get callee name

    Value getVar(const std::string& name);
    void setVar(const std::string& name, const Value& v);
    void pushScope();
    void popScope();
    static bool isTrue(const Value& v);
    static std::string formatFloat(double d);
    static Value toInt(const Value& v);
    static Value toFloat(const Value& v);
    static Value toStr(const Value& v);
    static Value toBool(const Value& v);
    static int compareValues(const Value& a, const Value& b);
    static Value tryConvertForCompare(const Value& a, const Value& b);
};

#endif
