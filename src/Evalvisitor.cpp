#include "Evalvisitor.h"
#include "Python3Lexer.h"
#include "Python3Parser.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cctype>

// ============== BigInt ==============
std::string BigInt::trimZeros(std::string s) {
    size_t i = 0;
    while (i < s.size() && s[i] == '0') i++;
    return i < s.size() ? s.substr(i) : "0";
}

int BigInt::compare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

std::string BigInt::add(const std::string& a, const std::string& b) {
    std::string res;
    int carry = 0;
    size_t i = a.size(), j = b.size();
    while (i || j || carry) {
        int sum = carry;
        if (i) sum += a[--i] - '0';
        if (j) sum += b[--j] - '0';
        res += char('0' + sum % 10);
        carry = sum / 10;
    }
    std::reverse(res.begin(), res.end());
    return trimZeros(res);
}

std::string BigInt::sub(const std::string& a, const std::string& b) {
    if (compare(a, b) < 0) return "0";
    std::string res;
    int borrow = 0;
    size_t i = a.size(), j = b.size();
    while (i) {
        int d = a[--i] - '0' - borrow;
        if (j) d -= b[--j] - '0';
        if (d < 0) { d += 10; borrow = 1; } else borrow = 0;
        res += char('0' + d);
    }
    std::reverse(res.begin(), res.end());
    return trimZeros(res);
}

std::string BigInt::mul(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0") return "0";
    std::vector<int> v(a.size() + b.size(), 0);
    for (size_t i = 0; i < a.size(); i++)
        for (size_t j = 0; j < b.size(); j++)
            v[i + j + 1] += (a[i] - '0') * (b[j] - '0');
    for (size_t k = v.size() - 1; k; k--) {
        v[k - 1] += v[k] / 10;
        v[k] %= 10;
    }
    std::string res;
    for (size_t k = (v[0] == 0 ? 1 : 0); k < v.size(); k++) res += char('0' + v[k]);
    return res;
}

std::string BigInt::divmod(const std::string& a, const std::string& b, std::string& rem) {
    if (b == "0") throw std::runtime_error("division by zero");
    int cmp = compare(a, b);
    if (cmp < 0) { rem = a; return "0"; }
    if (cmp == 0) { rem = "0"; return "1"; }
    std::string q, r = "";
    for (size_t i = 0; i < a.size(); i++) {
        r += a[i];
        r = trimZeros(r);
        int d = 0;
        for (int digit = 9; digit >= 0; digit--) {
            std::string prod = mul(b, std::string(1, '0' + digit));
            if (compare(r, prod) >= 0) {
                r = sub(r, prod);
                d = digit;
                break;
            }
        }
        q += char('0' + d);
    }
    rem = trimZeros(r);
    return trimZeros(q);
}

BigInt::BigInt(long long n) {
    if (n < 0) { negative = true; n = -n; } else negative = false;
    if (n == 0) { digits = "0"; return; }
    digits.clear();
    while (n) { digits += char('0' + n % 10); n /= 10; }
    std::reverse(digits.begin(), digits.end());
}

BigInt::BigInt(const std::string& s) {
    std::string t = s;
    while (!t.empty() && t[0] == ' ') t = t.substr(1);
    if (t.empty()) { negative = false; digits = "0"; return; }
    if (t[0] == '-') { negative = true; t = t.substr(1); } else negative = false;
    if (t[0] == '+') t = t.substr(1);
    digits = trimZeros(t);
    if (digits == "0") negative = false;
}

std::string BigInt::toString() const {
    return negative ? "-" + digits : digits;
}

long long BigInt::toLong() const {
    long long r = 0;
    for (char c : digits) r = r * 10 + (c - '0');
    return negative ? -r : r;
}

BigInt BigInt::operator-() const {
    BigInt r = *this;
    if (digits != "0") r.negative = !r.negative;
    return r;
}

BigInt BigInt::operator+(const BigInt& o) const {
    if (negative == o.negative) {
        BigInt r; r.negative = negative; r.digits = add(digits, o.digits); return r;
    }
    int c = compare(digits, o.digits);
    if (c == 0) return BigInt(0);
    BigInt r;
    if (c > 0) {
        r.negative = negative;
        r.digits = sub(digits, o.digits);
    } else {
        r.negative = o.negative;
        r.digits = sub(o.digits, digits);
    }
    return r;
}

BigInt BigInt::operator-(const BigInt& o) const {
    return *this + (-o);
}

BigInt BigInt::operator*(const BigInt& o) const {
    BigInt r; r.negative = (negative != o.negative); r.digits = mul(digits, o.digits);
    if (r.digits == "0") r.negative = false;
    return r;
}

BigInt BigInt::operator/(const BigInt& o) const {
    if (o.digits == "0") throw std::runtime_error("division by zero");
    std::string rem;
    std::string q = divmod(digits, o.digits, rem);
    bool neg = (negative != o.negative);
    if (neg && rem != "0") q = add(q, "1");  // floor division: -5//3 = -2
    BigInt r; r.negative = neg; r.digits = q;
    if (r.digits == "0") r.negative = false;
    return r;
}

BigInt BigInt::operator%(const BigInt& o) const {
    if (o.digits == "0") throw std::runtime_error("division by zero");
    std::string rem;
    divmod(digits, o.digits, rem);
    BigInt r; r.negative = negative; r.digits = rem;
    if (r.digits == "0") r.negative = false;
    return r;
}

bool BigInt::operator<(const BigInt& o) const {
    if (negative != o.negative) return negative;
    int c = compare(digits, o.digits);
    return negative ? c > 0 : c < 0;
}
bool BigInt::operator>(const BigInt& o) const { return o < *this; }
bool BigInt::operator<=(const BigInt& o) const { return !(o < *this); }
bool BigInt::operator>=(const BigInt& o) const { return !(*this < o); }
bool BigInt::operator==(const BigInt& o) const { return negative == o.negative && digits == o.digits; }
bool BigInt::operator!=(const BigInt& o) const { return !(*this == o); }

BigInt& BigInt::operator+=(const BigInt& o) { *this = *this + o; return *this; }
BigInt& BigInt::operator-=(const BigInt& o) { *this = *this - o; return *this; }
BigInt& BigInt::operator*=(const BigInt& o) { *this = *this * o; return *this; }
BigInt& BigInt::operator/=(const BigInt& o) { *this = *this / o; return *this; }
BigInt& BigInt::operator%=(const BigInt& o) { *this = *this % o; return *this; }

// Floor division and modulo: -5 // 3 = -2, -5 % 3 = 1; a % b = a - (a//b)*b
static BigInt floorDiv(BigInt a, BigInt b) {
    return a / b;  // BigInt::operator/ already does floor division
}
static BigInt floorMod(BigInt a, BigInt b) {
    return a - (a / b) * b;
}

// ============== Value helpers ==============
bool EvalVisitor::isTrue(const Value& v) {
    if (std::holds_alternative<PyNone>(v)) return false;
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    if (std::holds_alternative<BigInt>(v)) return std::get<BigInt>(v) != BigInt(0);
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0.0;
    if (std::holds_alternative<std::string>(v)) return !std::get<std::string>(v).empty();
    if (std::holds_alternative<std::shared_ptr<PyTuple>>(v)) return !std::get<std::shared_ptr<PyTuple>>(v)->elts.empty();
    return false;
}

std::string EvalVisitor::formatFloat(double d) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << d;
    return oss.str();
}

Value EvalVisitor::toInt(const Value& v) {
    if (std::holds_alternative<BigInt>(v)) return v;
    if (std::holds_alternative<double>(v)) return BigInt((long long)std::get<double>(v));
    if (std::holds_alternative<bool>(v)) return BigInt(std::get<bool>(v) ? 1 : 0);
    if (std::holds_alternative<std::string>(v)) {
        const std::string& s = std::get<std::string>(v);
        if (s.empty()) throw std::runtime_error("invalid literal for int()");
        size_t i = 0;
        if (s[i] == '+' || s[i] == '-') i++;
        for (; i < s.size(); i++) if (!std::isdigit((unsigned char)s[i])) throw std::runtime_error("invalid literal for int()");
        return BigInt(s);
    }
    throw std::runtime_error("cannot convert to int");
}

Value EvalVisitor::toFloat(const Value& v) {
    if (std::holds_alternative<double>(v)) return v;
    if (std::holds_alternative<BigInt>(v)) return (double)std::get<BigInt>(v).toLong();
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
    if (std::holds_alternative<std::string>(v)) return std::stod(std::get<std::string>(v));
    throw std::runtime_error("cannot convert to float");
}

Value EvalVisitor::toStr(const Value& v) {
    if (std::holds_alternative<std::string>(v)) return v;
    if (std::holds_alternative<BigInt>(v)) return std::get<BigInt>(v).toString();
    if (std::holds_alternative<double>(v)) return formatFloat(std::get<double>(v));
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "True" : "False";
    if (std::holds_alternative<PyNone>(v)) return "None";
    return "?";
}

Value EvalVisitor::toBool(const Value& v) {
    return isTrue(v);
}

int EvalVisitor::compareValues(const Value& a, const Value& b) {
    if (std::holds_alternative<BigInt>(a) && std::holds_alternative<BigInt>(b)) {
        BigInt x = std::get<BigInt>(a), y = std::get<BigInt>(b);
        if (x < y) return -1; if (x > y) return 1; return 0;
    }
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        double x = std::get<double>(a), y = std::get<double>(b);
        if (x < y) return -1; if (x > y) return 1; return 0;
    }
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
        return std::get<std::string>(a).compare(std::get<std::string>(b));
    }
    return -2;  // incomparable
}

Value EvalVisitor::tryConvertForCompare(const Value& a, const Value& b) {
    if (std::holds_alternative<PyNone>(a) || std::holds_alternative<PyNone>(b)) return PyNone{};
    if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) return PyNone{};
    try {
        if (std::holds_alternative<BigInt>(a) && std::holds_alternative<double>(b))
            return compareValues(a, toInt(b)) == 0;
        if (std::holds_alternative<double>(a) && std::holds_alternative<BigInt>(b))
            return compareValues(toInt(a), b) == 0;
        if (std::holds_alternative<BigInt>(a) && std::holds_alternative<bool>(b))
            return compareValues(a, toInt(b)) == 0;
        if (std::holds_alternative<bool>(a) && std::holds_alternative<BigInt>(b))
            return compareValues(toInt(a), b) == 0;
        if (std::holds_alternative<double>(a) && std::holds_alternative<bool>(b))
            return compareValues(a, toFloat(b)) == 0;
        if (std::holds_alternative<bool>(a) && std::holds_alternative<double>(b))
            return compareValues(toFloat(a), b) == 0;
        if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
            return std::get<bool>(a) == std::get<bool>(b);
    } catch (...) {}
    return PyNone{};
}

void EvalVisitor::pushScope() { scopes.push_back({}); }
void EvalVisitor::popScope() { scopes.pop_back(); }

Value EvalVisitor::getVar(const std::string& name) {
    for (int i = (int)scopes.size() - 1; i >= 0; i--) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    throw std::runtime_error("name '" + name + "' is not defined");
}

void EvalVisitor::setVar(const std::string& name, const Value& v) {
    for (int i = (int)scopes.size() - 1; i >= 0; i--) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) { scopes[i][name] = v; return; }
    }
    scopes.back()[name] = v;
}

// ============== Visitors ==============
std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    pushScope();
    for (auto* s : ctx->stmt()) visit(s);
    popScope();
    return nullptr;
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    std::string name = ctx->NAME()->getText();
    auto* paramsCtx = ctx->parameters()->typedargslist();
    std::vector<std::string> names;
    std::vector<Value> defaults;
    if (paramsCtx) {
        for (auto* tfp : paramsCtx->tfpdef())
            names.push_back(tfp->NAME()->getText());
        for (auto* t : paramsCtx->test()) {
            Value v = std::any_cast<Value>(visit(t));
            defaults.push_back(v);
        }
    }
    functions[name] = {names, defaults, ctx->suite()};
    return nullptr;
}

std::any EvalVisitor::visitParameters(Python3Parser::ParametersContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->simple_stmt()) return visit(ctx->simple_stmt());
    return visit(ctx->compound_stmt());
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visit(ctx->small_stmt());
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt()) return visit(ctx->expr_stmt());
    return visit(ctx->flow_stmt());
}

static std::string getSingleName(Python3Parser::TestlistContext* tl) {
    if (!tl || tl->test().size() != 1) return "";
    auto* f = tl->test(0)->or_test()->and_test(0)->not_test(0)->comparison()->arith_expr(0)->term(0)->factor(0)->atom_expr();
    if (f->atom()->NAME() && !f->trailer()) return f->atom()->NAME()->getText();
    return "";
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    auto testlists = ctx->testlist();
    if (ctx->augassign()) {
        std::string name = getSingleName(testlists[0]);
        if (!name.empty()) {
            Value left = getVar(name);
            Value right = std::any_cast<Value>(visit(testlists[1]));
            std::string op = ctx->augassign()->getText();
            Value res;
            if (op == "+=") {
                if (std::holds_alternative<BigInt>(left) && std::holds_alternative<BigInt>(right))
                    res = std::get<BigInt>(left) + std::get<BigInt>(toInt(right));
                else if (std::holds_alternative<double>(left) || std::holds_alternative<double>(right))
                    res = std::get<double>(toFloat(left)) + std::get<double>(toFloat(right));
                else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
                    res = std::get<std::string>(left) + std::get<std::string>(toStr(right));
                else
                    res = std::get<BigInt>(toInt(left)) + std::get<BigInt>(toInt(right));
            } else if (op == "-=") {
                if (std::holds_alternative<BigInt>(left)) res = std::get<BigInt>(left) - std::get<BigInt>(toInt(right));
                else res = std::get<double>(toFloat(left)) - std::get<double>(toFloat(right));
            } else if (op == "*=") {
                if (std::holds_alternative<std::string>(left) && std::holds_alternative<BigInt>(right)) {
                    std::string s; int n = std::get<BigInt>(right).toLong();
                    for (int j = 0; j < n; j++) s += std::get<std::string>(left);
                    res = s;
                } else if (std::holds_alternative<BigInt>(left) && std::holds_alternative<BigInt>(right))
                    res = std::get<BigInt>(left) * std::get<BigInt>(right);
                else
                    res = std::get<double>(toFloat(left)) * std::get<double>(toFloat(right));
            } else if (op == "/=")
                res = std::get<double>(toFloat(left)) / std::get<double>(toFloat(right));
            else if (op == "//=") {
                if (std::holds_alternative<BigInt>(left)) res = floorDiv(std::get<BigInt>(left), std::get<BigInt>(toInt(right)));
                else res = BigInt((long long)(std::get<double>(toFloat(left)) / std::get<double>(toFloat(right))));
            } else if (op == "%=") {
                if (std::holds_alternative<BigInt>(left)) res = floorMod(std::get<BigInt>(left), std::get<BigInt>(toInt(right)));
                else res = std::fmod(std::get<double>(toFloat(left)), std::get<double>(toFloat(right)));
            }
            setVar(name, res);
            return nullptr;
        }
        return nullptr;  // augassign but not single name (e.g. a[i] += x)
    }
    if (ctx->ASSIGN().empty()) {
        return visit(testlists[0]);
    }
    size_t n = testlists.size();
    Value rhs = std::any_cast<Value>(visit(testlists[n - 1]));
    std::vector<Value> rhsList;
    if (std::holds_alternative<std::shared_ptr<PyTuple>>(rhs)) {
        rhsList = std::get<std::shared_ptr<PyTuple>>(rhs)->elts;
    } else {
        rhsList.push_back(rhs);
    }
    size_t lhsCount = 0;
    for (size_t i = 0; i < n - 1; i++) lhsCount += testlists[i]->test().size();
    if (lhsCount == rhsList.size()) {
        size_t idx = 0;
        for (size_t i = 0; i < n - 1; i++) {
            for (auto* t : testlists[i]->test()) {
                std::string name = t->or_test()->and_test(0)->not_test(0)->comparison()->arith_expr(0)->term(0)->factor(0)->atom_expr()->atom()->NAME()->getText();
                setVar(name, rhsList[idx++]);
            }
        }
    } else if (lhsCount == 1 && rhsList.size() == 1) {
        setVar(getSingleName(testlists[0]), rhsList[0]);
    }
    return nullptr;
}

std::any EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    if (ctx->break_stmt()) return visit(ctx->break_stmt());
    if (ctx->continue_stmt()) return visit(ctx->continue_stmt());
    return visit(ctx->return_stmt());
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    throw FlowType::Break;
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    throw FlowType::Continue;
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    if (ctx->testlist()) {
        Value v = std::any_cast<Value>(visit(ctx->testlist()));
        throw std::make_pair(FlowType::Return, std::move(v));
    }
    throw std::make_pair(FlowType::Return, Value(PyNone{}));
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    if (ctx->if_stmt()) return visit(ctx->if_stmt());
    if (ctx->while_stmt()) return visit(ctx->while_stmt());
    return visit(ctx->funcdef());
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    size_t i = 0;
    for (; i < ctx->test().size(); i++) {
        Value c = std::any_cast<Value>(visit(ctx->test(i)));
        if (isTrue(c)) { visit(ctx->suite(i)); return nullptr; }
    }
    if (ctx->ELSE()) visit(ctx->suite(i));
    return nullptr;
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    while (true) {
        Value c = std::any_cast<Value>(visit(ctx->test()));
        if (!isTrue(c)) break;
        try {
            visit(ctx->suite());
        } catch (FlowType f) {
            if (f == FlowType::Break) break;
            if (f == FlowType::Continue) continue;
            throw;
        }
    }
    return nullptr;
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    if (ctx->simple_stmt()) return visit(ctx->simple_stmt());
    for (auto* s : ctx->stmt()) visit(s);
    return nullptr;
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return visit(ctx->or_test());
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    for (size_t i = 0; i < ctx->and_test().size(); i++) {
        Value v = std::any_cast<Value>(visit(ctx->and_test(i)));
        if (isTrue(v)) return v;
        if (i + 1 < ctx->and_test().size()) continue;
        return v;
    }
    return Value(false);
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    for (size_t i = 0; i < ctx->not_test().size(); i++) {
        Value v = std::any_cast<Value>(visit(ctx->not_test(i)));
        if (!isTrue(v)) return v;
        if (i + 1 < ctx->not_test().size()) continue;
        return v;
    }
    return Value(true);
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->NOT()) return Value(!isTrue(std::any_cast<Value>(visit(ctx->not_test()))));
    return visit(ctx->comparison());
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    auto arith = ctx->arith_expr();
    if (arith.size() == 1) return visit(arith[0]);
    bool result = true;
    for (size_t i = 0; i < ctx->comp_op().size(); i++) {
        Value left = std::any_cast<Value>(visit(arith[i]));
        Value right = std::any_cast<Value>(visit(arith[i + 1]));
        auto* op = ctx->comp_op(i);
        bool cmp = false;
        if (op->LESS_THAN()) cmp = compareValues(left, right) < 0;
        else if (op->GREATER_THAN()) cmp = compareValues(left, right) > 0;
        else if (op->LT_EQ()) cmp = compareValues(left, right) <= 0;
        else if (op->GT_EQ()) cmp = compareValues(left, right) >= 0;
        else if (op->EQUALS()) {
            Value conv = tryConvertForCompare(left, right);
            if (std::holds_alternative<PyNone>(conv)) cmp = (compareValues(left, right) == 0);
            else cmp = std::get<bool>(conv);
        } else if (op->NOT_EQ_2()) {
            Value conv = tryConvertForCompare(left, right);
            if (std::holds_alternative<PyNone>(conv)) cmp = (compareValues(left, right) != 0);
            else cmp = !std::get<bool>(conv);
        }
        result = result && cmp;
        if (!result) return Value(false);
    }
    return Value(result);
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    Value v = std::any_cast<Value>(visit(ctx->term(0)));
    for (size_t i = 0; i < ctx->addorsub_op().size(); i++) {
        Value r = std::any_cast<Value>(visit(ctx->term(i + 1)));
        bool sub = ctx->addorsub_op(i)->MINUS() != nullptr;
        if (std::holds_alternative<BigInt>(v) && std::holds_alternative<BigInt>(r)) {
            v = sub ? std::get<BigInt>(v) - std::get<BigInt>(r) : std::get<BigInt>(v) + std::get<BigInt>(r);
        } else if (std::holds_alternative<std::string>(v) && std::holds_alternative<std::string>(r) && !sub) {
            v = std::get<std::string>(v) + std::get<std::string>(r);
        } else {
            double a = std::holds_alternative<BigInt>(v) ? (double)std::get<BigInt>(v).toLong() : std::get<double>(toFloat(v));
            double b = std::holds_alternative<BigInt>(r) ? (double)std::get<BigInt>(r).toLong() : std::get<double>(toFloat(r));
            v = sub ? a - b : a + b;
        }
    }
    return v;
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    Value v = std::any_cast<Value>(visit(ctx->factor(0)));
    for (size_t i = 0; i < ctx->muldivmod_op().size(); i++) {
        Value r = std::any_cast<Value>(visit(ctx->factor(i + 1)));
        auto* op = ctx->muldivmod_op(i);
        if (op->STAR()) {
            if (std::holds_alternative<BigInt>(v) && std::holds_alternative<BigInt>(r))
                v = std::get<BigInt>(v) * std::get<BigInt>(r);
            else if (std::holds_alternative<std::string>(v) && std::holds_alternative<BigInt>(r)) {
                std::string s; int n = std::get<BigInt>(r).toLong();
                for (int j = 0; j < n; j++) s += std::get<std::string>(v);
                v = s;
            } else
                v = std::get<double>(toFloat(v)) * std::get<double>(toFloat(r));
        } else if (op->DIV())
            v = std::get<double>(toFloat(v)) / std::get<double>(toFloat(r));
        else if (op->IDIV()) {
            if (std::holds_alternative<BigInt>(v) && std::holds_alternative<BigInt>(r))
                v = floorDiv(std::get<BigInt>(v), std::get<BigInt>(r));
            else
                v = BigInt((long long)(std::get<double>(toFloat(v)) / std::get<double>(toFloat(r))));
        } else if (op->MOD()) {
            if (std::holds_alternative<BigInt>(v) && std::holds_alternative<BigInt>(r))
                v = floorMod(std::get<BigInt>(v), std::get<BigInt>(r));
            else
                v = std::fmod(std::get<double>(toFloat(v)), std::get<double>(toFloat(r)));
        }
    }
    return v;
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    Value v;
    if (ctx->atom_expr())
        v = std::any_cast<Value>(visit(ctx->atom_expr()));
    else
        v = std::any_cast<Value>(visit(ctx->factor()));
    if (ctx->MINUS()) return std::holds_alternative<BigInt>(v) ? Value(-std::get<BigInt>(v)) : Value(-std::get<double>(toFloat(v)));
    if (ctx->ADD()) return v;
    return v;
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    if (ctx->trailer()) {
        currentAtomExpr = ctx;
        auto r = visit(ctx->trailer());
        currentAtomExpr = nullptr;
        return r;
    }
    return visit(ctx->atom());
}

std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    if (!currentAtomExpr || !currentAtomExpr->atom()->NAME()) return nullptr;
    std::string funcName = currentAtomExpr->atom()->NAME()->getText();
    auto it = functions.find(funcName);
    if (it == functions.end()) {
        if (funcName == "print") {
            auto* arglist = ctx->arglist();
            std::vector<std::string> parts;
            if (arglist) {
                for (size_t i = 0; i < arglist->argument().size(); i++) {
                    auto* arg = arglist->argument(i);
                    Value v = std::any_cast<Value>(visit(arg->test(0)));
                    parts.push_back(std::get<std::string>(toStr(v)));
                }
            }
            for (size_t i = 0; i < parts.size(); i++) {
                if (i) std::cout << " ";
                std::cout << parts[i];
            }
            std::cout << "\n";
            return Value(PyNone{});
        }
        if (funcName == "int") {
            Value v = std::any_cast<Value>(visit(ctx->arglist()->argument(0)->test(0)));
            return toInt(v);
        }
        if (funcName == "float") {
            Value v = std::any_cast<Value>(visit(ctx->arglist()->argument(0)->test(0)));
            return toFloat(v);
        }
        if (funcName == "str") {
            Value v = std::any_cast<Value>(visit(ctx->arglist()->argument(0)->test(0)));
            return toStr(v);
        }
        if (funcName == "bool") {
            Value v = std::any_cast<Value>(visit(ctx->arglist()->argument(0)->test(0)));
            return toBool(v);
        }
    } else {
        auto& [paramNames, defaultValues, suiteCtx] = it->second;
        std::map<std::string, Value> kwargs;
        std::vector<Value> args;
        if (ctx->arglist()) {
            auto* al = ctx->arglist();
            for (size_t i = 0; i < al->argument().size(); i++) {
                auto* arg = al->argument(i);
                if (arg->ASSIGN()) {
                    std::string name = arg->test(0)->or_test()->and_test(0)->not_test(0)->comparison()->arith_expr(0)->term(0)->factor(0)->atom_expr()->atom()->NAME()->getText();
                    kwargs[name] = std::any_cast<Value>(visit(arg->test(1)));
                } else {
                    args.push_back(std::any_cast<Value>(visit(arg->test(0))));
                }
            }
        }
        pushScope();
        size_t p = paramNames.size();
        size_t defStart = p - defaultValues.size();
        for (size_t i = 0; i < p; i++) {
            if (i < args.size())
                setVar(paramNames[i], args[i]);
            else if (kwargs.count(paramNames[i]))
                setVar(paramNames[i], kwargs[paramNames[i]]);
            else if (i >= defStart)
                setVar(paramNames[i], defaultValues[i - defStart]);
        }
        try {
            visit(suiteCtx);
            popScope();
            return Value(PyNone{});
        } catch (const std::pair<FlowType, Value>& ret) {
            popScope();
            if (ret.first == FlowType::Return) return ret.second;
            throw;
        }
    }
    return nullptr;
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NAME()) {
        return getVar(ctx->NAME()->getText());
    }
    if (ctx->NUMBER()) {
        std::string text = ctx->NUMBER()->getText();
        if (text.find('.') != std::string::npos)
            return Value(std::stod(text));
        return Value(BigInt(text));
    }
    if (ctx->STRING().size() > 0) {
        std::string s;
        for (auto* node : ctx->STRING()) {
            std::string t = node->getText();
            if (t.size() >= 2 && (t[0] == '"' || t[0] == '\'')) {
                char q = t[0];
                t = t.substr(1, t.size() - 2);
                for (size_t i = 0; i < t.size(); i++) {
                    if (t[i] == '\\' && i + 1 < t.size()) {
                        if (t[i+1] == 'n') { s += '\n'; i++; continue; }
                        if (t[i+1] == 't') { s += '\t'; i++; continue; }
                        if (t[i+1] == '\\' || t[i+1] == q) { s += t[i+1]; i++; continue; }
                    }
                    s += t[i];
                }
            } else s += t;
        }
        return Value(s);
    }
    if (ctx->NONE()) return Value(PyNone{});
    if (ctx->TRUE()) return Value(true);
    if (ctx->FALSE()) return Value(false);
    if (ctx->format_string()) return visit(ctx->format_string());
    if (ctx->OPEN_PAREN() && ctx->test()) return visit(ctx->test());
    return nullptr;
}

static void appendFStrLiteral(std::string& res, const std::string& t) {
    for (size_t j = 0; j < t.size(); j++) {
        if (t[j] == '{' && j+1 < t.size() && t[j+1] == '{') { res += '{'; j++; continue; }
        if (t[j] == '}' && j+1 < t.size() && t[j+1] == '}') { res += '}'; j++; continue; }
        res += t[j];
    }
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    std::string res;
    auto* tree = static_cast<antlr4::tree::ParseTree*>(ctx);
    for (antlr4::tree::ParseTree* child : tree->children) {
        if (auto* lit = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            if (lit->getSymbol()->getType() == Python3Lexer::FORMAT_STRING_LITERAL)
                appendFStrLiteral(res, lit->getText());
        } else if (auto* tl = dynamic_cast<Python3Parser::TestlistContext*>(child)) {
            Value v = std::any_cast<Value>(visit(tl));
            if (std::holds_alternative<bool>(v)) res += std::get<bool>(v) ? "True" : "False";
            else if (std::holds_alternative<BigInt>(v)) res += std::get<BigInt>(v).toString();
            else if (std::holds_alternative<double>(v)) res += formatFloat(std::get<double>(v));
            else if (std::holds_alternative<std::string>(v)) res += std::get<std::string>(v);
            else if (std::holds_alternative<PyNone>(v)) res += "None";
        }
    }
    return Value(res);
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    if (ctx->test().size() == 1) return visit(ctx->test(0));
    auto tup = std::make_shared<PyTuple>();
    for (auto* t : ctx->test()) tup->elts.push_back(std::any_cast<Value>(visit(t)));
    return Value(tup);
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    return visit(ctx->test(0));
}
