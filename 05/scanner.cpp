#include "scanner.h"

std::unordered_map<TokenKind, std::string> tokenToString {
    {TokenKind::Keyword, "Keyword"},
    {TokenKind::Identifier, "Identifier"},
    {TokenKind::StringLiteral, "StringLiteral"},
    {TokenKind::IntegerLiteral, "IntegerLiteral"},
    {TokenKind::DecimalLiteral, "DecimalLiteral"},
    {TokenKind::Seperator, "Seperator"},
    {TokenKind::Operator, "Operator"},
    {TokenKind::Eof, "Eof"},
};

std::string toString(TokenKind kind) {
    auto it = tokenToString.find(kind);
    if (it != tokenToString.end()) {
        return it->second;
    }

    return "Unknow";
}

std::unordered_map<std::string, KeywordKind> Scanner::KeywordMap {
    {"function",    KeywordKind::Function},
    {"class",       KeywordKind::Class},
    {"break",       KeywordKind::Break},
    {"delete",      KeywordKind::Delete},
    {"return",      KeywordKind::Return},
    {"case",        KeywordKind::Case},
    {"do",          KeywordKind::Do},
    {"if",          KeywordKind::If},
    {"switch",      KeywordKind::Switch},
    {"var",         KeywordKind::Var},
    {"catch",       KeywordKind::Catch},
    {"else",        KeywordKind::Else},
    {"in",          KeywordKind::In},
    {"this",        KeywordKind::This},
    {"void",        KeywordKind::Void},
    {"continue",    KeywordKind::Continue},
    {"false",       KeywordKind::False},
    {"instanceof",  KeywordKind::Instanceof},
    {"throw",       KeywordKind::Throw},
    {"while",       KeywordKind::While },
    {"debugger",    KeywordKind::Debugger},
    {"finally",     KeywordKind::Finally},
    {"new",         KeywordKind::New},
    {"true",        KeywordKind::True},
    {"with",        KeywordKind::With},
    {"default",     KeywordKind::Default},
    {"for",         KeywordKind::For},
    {"null",        KeywordKind::Null},
    {"try",         KeywordKind::Try},
    {"typeof",      KeywordKind::Typeof},
    //下面这些用于严格模式
    {"implements",  KeywordKind::Implements},
    {"let",         KeywordKind::Let},
    {"private",     KeywordKind::Private},
    {"public",      KeywordKind::Public},
    {"yield",       KeywordKind::Yield},
    {"interface",   KeywordKind::Interface},
    {"package",     KeywordKind::Package},
    {"protected",   KeywordKind::Protected},
    {"static",      KeywordKind::Static},
    //类型
    //{"number",      KeywordKind::Number},
    {"string",      KeywordKind::String},
    {"boolean",     KeywordKind::Boolean},
    {"any",         KeywordKind::Any},
    {"symbol",      KeywordKind::Symbol},
    //值
    {"undefined",      KeywordKind::Undefined},
};
