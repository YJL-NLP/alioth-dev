#ifndef __xengine_cpp__
#define __xengine_cpp__

#include "xengine.hpp"

namespace alioth {

Jsonz Xengine::__table = JNull;

void Xengine::begin( std::istream& is, bool li ) {
    state = 1;
    stay = false;
    synst = 1;
    limit = li;
    pis = &is;
    T = token(VT::R_BEG);
    ret.clear();
    ret << std::move(T);
    T.bl = begl = 1;
    T.bc = begc = 1;
    pre = pis->peek();
}

void Xengine::goon() {
    if( stay ) {stay = false;return;}
    if( pre == '\n' ) begl += begc = 1;
    else begc += 1;
    T.tx += pis->get();
    pre = pis->peek();
};

void Xengine::check(VT t,bool s ) {
    if( s ) goon();
    
    T.id = t;
    T.el = begl;
    T.ec = begc;
    ret << std::move(T);

    T.tx.clear();
    T.bl = begl;
    T.bc = begc;
    state = ((pre==EOF)?0:1);
    stay = true;

    /*微型语法分析器*/
    if( limit ) switch(t) {
        case VT::SPACE:case VT::COMMENT: break;
        case VT::MODULE:
            if( synst == 1 ) synst = 2;
            else if( synst == 9 ) synst = 5;
            else state = -1;
            break;
        case VT::LABEL:
            if( synst == 2 ) synst = 3;
            else if( synst == 4 ) synst = 5;
            else if( synst == 5 ) synst = 5;
            else if( synst == 6 ) synst = 7;
            else if( synst == 7 ) synst = 5;
            else if( synst == 8 ) synst = 5;
            else if( synst == 10 ) synst = 11;
            else state = -1;
            break;
        case VT::COLON:
            if( synst == 3 ) synst = 4;
            else if( synst == 11 ) synst = 4;
            else state = -1;
            break;
        case VT::AT:
            if( synst == 5 ) synst = 6;
            else state = -1;
            break;
        case VT::MEMBER:case VT::iCHAR:case VT::iSTRING:
            if( synst == 6 ) synst = 7;
            else state = -1;
            break;
        case VT::AS:
            if( synst == 5 ) synst = 8;
            else if( synst == 7 ) synst = 8;
            else state = -1;
            break;
        case VT::iTHIS:
            if( synst == 8 ) synst = 9;
            else state = -1;
            break;
        case VT::ENTRY:
            if( synst == 3 ) synst = 10;
            else state = -1;
            break;
        default:state = -1;
    }
};

void Xengine::test(VT v, int o ){
    T.id = v;
    fixed = __table[(int)v];
    off = o+1;
    state = 2;      //用于单词测试的状态
};

void Xengine::prefix( const char* s, int o, int t ) {
    fixed = s;
    off = o+1;
    target = t;
    state = 5;      //用于前缀测试的状态
};

void Xengine::assign(VT h, VT m) {
    T.id = m;
    hit = h;
    state = 6;      //用于赋值测试的状态
};

void Xengine::sequence( VT h ) {
        T.id = h;
        state = 7;
}

bool Xengine::islabel( int c ) {
    return isalnum(c) or c == '_';
}
bool Xengine::islabelb( int c ) {
    return isalpha(c) or c == '_';
}

tokens Xengine::extractTokens( std::istream& is, bool limit ) {

    for( begin(is,limit); state > 0; goon() ) switch( state ) {
        case 1:
            if( pre == EOF ) state = 0;
            else if( isspace(pre) ) state = 3;
            else if( pre == 'a' ) state = 10;
            else if( pre == 'b' ) state = 11;
            else if( pre == 'c' ) state = 12;
            else if( pre == 'd' ) state = 16;
            else if( pre == 'e' ) state = 18;
            else if( pre == 'f' ) state = 20;
            else if( pre == 'i' ) state = 22;
            else if( pre == 'l' ) state = 24;
            else if( pre == 'm' ) state = 25;
            else if( pre == 'n' ) state = 27;
            else if( pre == 'o' ) state = 28;
            else if( pre == 'p' ) state = 29;
            else if( pre == 'r' ) state = 30;
            else if( pre == 's' ) state = 32;
            else if( pre == 't' ) state = 33;
            else if( pre == 'u' ) state = 34;
            else if( pre == 'v' ) state = 36;
            else if( islabelb(pre) ) state = 4;
            else if( pre == '~' ) check(VT::bREV,true);
            else if( pre == '!' ) state = 57;//assign(VT::NE,VT::FORCE);
            else if( pre == '@' ) check(VT::AT,true);
            else if( pre == '#' ) check(VT::WHERE,true);
            else if( pre == '$' ) check(VT::CONV,true);
            else if( pre == '%' ) assign(VT::ASSIGN_MOL,VT::MOL);
            else if( pre == '^' ) assign(VT::ASSIGN_bXOR,VT::bXOR);
            else if( pre == '&' ) assign(VT::ASSIGN_bAND,VT::bAND);
            else if( pre == '*' ) assign(VT::ASSIGN_MUL,VT::MUL);
            else if( pre == '(' ) check(VT::OPENA,true);
            else if( pre == ')' ) check(VT::CLOSEA,true);
            else if( pre == '-' ) state = 38;
            else if( pre == '+' ) state = 39;
            else if( pre == '=' ) assign(VT::EQ,VT::ASSIGN);
            else if( pre == '[' ) check(VT::OPENL,true);
            else if( pre == ']' ) check(VT::CLOSEL,true);
            else if( pre == '{' ) check(VT::OPENS,true);
            else if( pre == '}' ) check(VT::CLOSES,true);
            else if( pre == '|' ) assign(VT::ASSIGN_bOR,VT::bOR);
            else if( pre == ';' ) check(VT::SEMI,true);
            else if( pre == ':' ) state = 40;
            else if( pre == ',' ) check(VT::COMMA,true);
            else if( pre == '?' ) check(VT::ASK,true);
            else if( pre == '<' ) state = 41;
            else if( pre == '>' ) state = 42;
            else if( pre == '.' ) state = 43;
            else if( pre == '/' ) state = 45;
            else if( pre == '"' ) sequence(VT::iSTRING);
            else if( pre == '\'') sequence(VT::iCHAR);
            else if( pre == '0' ) state = 9;
            else if( pre >= '1' and pre <= '9' ) {T.id = VT::iINTEGERn; state = 50;}
            else check(VT::R_ERR,true);
            break;
        case 2:
            if( pre and pre == fixed[off] ) off += 1;
            else if( islabel(pre) ) state = 4;
            else if( off == (long long)fixed.size() ) check(T.id,false);
            else check(VT::LABEL,false);
            break;
        case 3:
            if( !isspace(pre) ) check(VT::SPACE,false); 
            break;
        case 4:
            if( !islabel(pre) ) check(VT::LABEL,false);
            break;
        case 5:
            if( pre and pre == fixed[off] ) off += 1;
            else if( off == (long long)fixed.size() ) {state = target;stay = true;}
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 6:
            if( pre == '=' ) check(hit,true);
            else check(T.id,false);
            break;
        case 7:
            if( T.id == VT::iSTRING and pre == '\"' ) check(T.id,true);
            else if( T.id == VT::iCHAR and pre == '\'' ) check(T.id,true);
            else if( pre == '\0' or pre == EOF ) check(VT::R_ERR,false);
            else if( pre == '\\' ) state = 8;
            break;
        case 8:
            if( pre == '\0' or pre == EOF ) check(VT::R_ERR,false);
            else state = 7;
            break;
        case 9:
            if( pre == 'b') {T.id = VT::iINTEGERb; state = 52;}
            else if( pre == 'o' ) {T.id = VT::iINTEGERo; state = 51;}
            else if( pre == 'x' ) {T.id = VT::iINTEGERh; state = 49;}
            else {T.id = VT::iINTEGERn; state = 50;stay = true;}
            break;
        case 10:
            if( pre == 's' ) state = 15;
            else if( pre == 'n' ) test(VT::AND,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 11:
            if( pre == 'r' ) test(VT::BREAK,1);
            else if( pre == 'o' ) test(VT::BOOL,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 12:
            if( pre == 'o' ) state = 13;
            else if( pre == 'l' ) test(VT::CLASS,1);
            else if( pre == 'a' ) test(VT::CASE,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 13:
            if( pre == 'n' ) state = 14;
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 14:
            if( pre == 's' ) test(VT::CONST,3);
            else if( pre == 't' ) test(VT::CONTINUE,3);
            else check(VT::LABEL,false);
            break;
        case 15:
            if( pre == 's' ) test(VT::ASSUME,2);
            else if( pre == '!' ) check(VT::TREAT,true);
            else if( islabel(pre) ) state = 4;
            else check(VT::AS,false);
            break;
        case 16:
            if( pre == 'e' ) state = 17;
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 17:
            if( pre == 'l' ) test(VT::DELETE,2);
            else if( pre == 'f' ) test(VT::DEFAULT,2);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 18:
            if( pre == 'n' ) state = 19;
            else if( pre == 'l' ) test(VT::ELSE,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 19:
            if( pre == 'u' ) test(VT::ENUM,2);
            else if( pre == 't' ) test(VT::ENTRY,2);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 20:
            if( pre == 'a' ) test(VT::iFALSE,1);
            else if( pre == 'l' ) prefix("float",1,21);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 21:
            if( pre == '3' ) test(VT::FLOAT32,5);
            else if( pre == '6' ) test(VT::FLOAT64,5);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 22:
            if( pre == 'f' ) test(VT::IF,1);
            else if( pre == 'n' ) prefix("int",1,23);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 23:
            if( pre == '8' ) test(VT::INT8,3);
            else if( pre == '1' ) test(VT::INT16,3);
            else if( pre == '3' ) test(VT::INT32,3);
            else if( pre == '6' ) test(VT::INT64,3);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 24:
            if( pre == 'o' ) test(VT::LOOP,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 25:
            if( pre == 'e' ) prefix("met",1,58);
            else if( pre == 'o' ) prefix("mod",1,26);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 26:
            if( pre == 'u' ) test(VT::MODULE,3);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 27:
            if( pre == 'e' ) test(VT::NEW,1);
            else if( pre == 'i' ) test(VT::NIL,1);
            else if( pre == 'o' ) test(VT::NOT,1);
            else if( pre == 'u' ) test(VT::iNULL,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 28:
            if( pre == 'r' ) test(VT::OR,1);
            else if( pre == 't' ) test(VT::OTHERWISE,1);
            else if( pre == 'p' ) test(VT::OPERATOR,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 29:
            if( pre == 'r' ) test(VT::PRIVATE,1);
            else if( pre == 't' ) test(VT::PTR,1);
            else if( pre == 'u' ) test(VT::PUBLIC,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 30:
            if( pre == 'e' ) state = 31;
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 31:
            if( pre == 'f' ) test(VT::REF,2);
            else if( pre == 't' ) test(VT::RETURN,2);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 32:
            if( pre == 'w' ) test(VT::SWITCH,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 33:
            if( pre == 'h' ) test(VT::iTHIS,1);
            else if( pre == 'r' ) test(VT::iTRUE,1);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 34:
            if( pre == 'i' ) prefix("uint",1,35);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 35:
            if( pre == '8' ) test(VT::UINT8,4);
            else if( pre == '1' ) test(VT::UINT16,4);
            else if( pre == '3' ) test(VT::UINT32,4);
            else if( pre == '6' ) test(VT::UINT64,4);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 36:
            if( pre == 'a' ) state = 37;
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 37:
            if( pre == 'r' ) test(VT::VAR,2);
            else if( pre == 'l' ) test(VT::VAL,2);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
        case 38:
            if( pre == '-' ) check(VT::DECRESS,true);
            else if( pre == '=' ) check(VT::ASSIGN_MINUS,true);
            else check(VT::MINUS,false);
            break;
        case 39:
            if( pre == '+' ) check(VT::INCRESS,true);
            else if( pre == '=' ) check(VT::ASSIGN_PLUS,true);
            else check(VT::PLUS,false);
            break;
        case 40:
            if( pre == ':' ) check(VT::SCOPE,true);
            else check(VT::COLON,false);
            break;
        case 41:
            if( pre == '<' ) assign(VT::ASSIGN_SHL,VT::SHL);
            else if( pre == '=' ) check(VT::LE,true);
            else check(VT::LT,false);
            break;
        case 42:
            if( pre == '>' ) assign(VT::ASSIGN_SHR,VT::SHR);
            else if( pre == '=' ) check(VT::GE,true);
            else check(VT::GT,false);
            break;
        case 43:
            if( pre == '.' ) state = 44;
            else check(VT::MEMBER,false);
            break;
        case 44:
            if( pre == '.' ) check(VT::ETC,true);
            else check(VT::RANGE,false);
            break;
        case 45:
            if( pre == '/' ) state = 46;
            else if( pre == '*' ) state = 47;
            else if( pre == '=' ) check(VT::ASSIGN_DIV,true);
            else check(VT::DIV,false);
            break;
        case 46:
            if( pre == '\n' or pre == EOF or pre == '\0' ) check(VT::COMMENT,false);
            break;
        case 47:
            if( pre == '*' ) state = 48;
            else if( pre == EOF or pre == '\0' ) check(VT::R_ERR,false);
            break;
        case 48:
            if( pre == '/' ) check(VT::COMMENT,true);
            else if( pre != '*' ) state = 47;
            else if( pre == EOF or pre == '\0' ) check(VT::R_ERR,false);
            break;
        case 49:
            if( (pre >= 'a' and pre <= 'f') or (pre >= 'A' and pre <= 'F') ) continue;
            [[fallthrough]];
        case 50:
            if( pre == '8' or pre == '9' ) continue;
            else if( pre == '.' ) state = 53;
            else if( pre == 'e' ) state = 55;
            [[fallthrough]];
        case 51:
            if( pre >= '2' and pre <= '7' ) continue;
            [[fallthrough]];
        case 52:
            if( pre == '0' or pre == '1' or pre == '\'' ) continue;
            else if( isalpha(pre) ) check(VT::R_ERR,true);
            else check(T.id,false);
            break;
        case 53:
            if( pre >= '0' and pre <= '9' ) state = 54;
            else check(VT::R_ERR,true);
            break;
        case 54:
            if( pre >= '0' and pre <= '9' ) continue;
            else if( pre == 'e' ) state = 55;
            else if( T.tx.find('\'') == std::string::npos ) check(VT::iFLOAT,false);
            else check(VT::iFLOAT,false);
            break;
        case 55:
            if( pre == '-' or pre == '+' ) state = 56;
            else if( pre >= '0' and pre <= '9' ) state = 56;
            else check(VT::R_ERR,true);
            break;
        case 56:
            if( pre >= '0' and pre <= '9' ) continue;
            else if( T.tx.find('\'') == std::string::npos ) check(VT::iFLOAT,false);
            else check(VT::R_ERR,false);
            break;
        case 57:
            if( pre == '=' ) check(VT::NE,true);
            else if( pre == '!' ) check(VT::EXCEPTION,true);
            else check(VT::FORCE,false);
            break;
        case 58:
            if( pre == 'a' ) test(VT::META,3);
            else if( pre == 'h' ) test(VT::METHOD,3);
            else if( islabel(pre) ) state = 4;
            else check(VT::LABEL,false);
            break;
    }

    if( state < 0 ) {
        ret[-1].id = VT::R_END;
    } else {
        ret << token(VT::R_END);
        ret[-1].bl = ret[-1].el = begl;
        ret[-1].bc = ret[-1].ec = begc;
    }
    return std::move(ret);
}

void Xengine::init() {
    if( __table.is(JNull) ) {
        __table = Jsonz(JArray);

        __table[(int)VT::MODULE] = "module";
        __table[(int)VT::ENTRY] = "entry";
        __table[(int)VT::VAR] = "var";
        __table[(int)VT::PTR] = "ptr";
        __table[(int)VT::REF] = "ref";
        __table[(int)VT::VAL] = "val";
        __table[(int)VT::METHOD] = "method";
        __table[(int)VT::CLASS] = "class";
        __table[(int)VT::ENUM] = "enum";
        __table[(int)VT::OPERATOR] = "operator";
        __table[(int)VT::ASM] = "asm";
        __table[(int)VT::INT8] = "int8";
        __table[(int)VT::INT16] = "int16";
        __table[(int)VT::INT32] = "int32";
        __table[(int)VT::INT64] = "int64";
        __table[(int)VT::UINT8] = "uint8";
        __table[(int)VT::UINT16] = "uint16";
        __table[(int)VT::UINT32] = "uint32";
        __table[(int)VT::UINT64] = "uint64";
        __table[(int)VT::FLOAT32] = "float32";
        __table[(int)VT::FLOAT64] = "float64";
        __table[(int)VT::BOOL] = "bool";
        __table[(int)VT::iNULL] = "null";
        __table[(int)VT::NIL] = "nil";
        __table[(int)VT::iTRUE] = "true";
        __table[(int)VT::iFALSE] = "false";
        __table[(int)VT::iTHIS] = "this";
        __table[(int)VT::CONST] = "const";
        __table[(int)VT::META] = "meta";
        __table[(int)VT::PUBLIC] = "public";
        __table[(int)VT::PRIVATE] = "private";
        __table[(int)VT::CDECL] = "cdecl";
        __table[(int)VT::STDCALL] = "stdcall";
        __table[(int)VT::FASTCALL] = "fastcall";
        __table[(int)VT::THISCALL] = "thiscall";
        __table[(int)VT::ASSUME] = "assume";
        __table[(int)VT::OTHERWISE] = "otherwise";
        __table[(int)VT::IF] = "if";
        __table[(int)VT::ELSE] = "else";
        __table[(int)VT::LOOP] = "loop";
        __table[(int)VT::BREAK] = "break";
        __table[(int)VT::CONTINUE] = "continue";
        __table[(int)VT::RETURN] = "return";
        __table[(int)VT::SWITCH] = "switch";
        __table[(int)VT::CASE] = "case";
        __table[(int)VT::DEFAULT] = "default";
        __table[(int)VT::NEW] = "new";
        __table[(int)VT::DELETE] = "delete";
        __table[(int)VT::AND] = "and";
        __table[(int)VT::OR] = "or";
        __table[(int)VT::NOT] = "not";
        __table[(int)VT::AS] = "as";
    }
}

Xengine::Xengine() {
    init();
}

tokens Xengine::parseSourceCode( std::istream& is ) {
    return extractTokens(is,false);
}

tokens Xengine::parseModuleSignature( std::istream& is ) {
    return extractTokens(is,true);
}

std::string Xengine::written( const token& t ) {
    init();
    auto& str = __table.at((int)t.id);
    if( !null(&str) and str.is(JString) ) return str;
    return t.tx;
}

bool Xengine::isLabel( const std::string& s ) {
    if( s.empty() ) return false;
    if( !islabelb(s[0]) ) return false;
    for( auto& c : s )
        if( !islabel(c) ) 
            return false;
    return true;
}

std::string Xengine::extractText( const token& t ) {
    if( !t.is(VT::iSTRING,VT::iCHAR) ) return t.tx;
    std::string ret;
    for( size_t i = 1; i < t.tx.size(); i++ ) switch( char c = t.tx[i]; c ) {
        case '\'': 
            if( t.is(VT::iCHAR) ) i = t.tx.size();
            else ret += c;
            break;
        case '"': 
            if( t.is(VT::iSTRING) )i = t.tx.size();
            else ret += c;
            break; 
        case '\\':
            c = t.tx[++i];
            switch( c ) {
                case '0':ret += '\0';break;
                case 'a':ret += '\a';break;
                case 'b':ret += '\b';break;
                case 'e':ret += '\e';break;
                case 'n':ret += '\n';break;
                case 'r':ret += '\r';break;
                case 't':ret += '\t';break;
                case 'x': {
                    c = 0;
                    for( int x = 0; x < 2; x++ ) {
                        c <<= 4;
                        if(char x = t.tx[++i]; x >= '0' and x <= '9' ) c |= x-'0';
                        else if( x >= 'a' and x <= 'z' ) c |= x-'a'+0x0a;
                        else if( x >= 'A' and x <= 'Z' ) c |= x-'A'+0x0a;
                        else c = -1;
                    }
                    ret += c;
                } break;
                default: ret += c;break;
            }
            break;
        default:ret += c;break;
    }
    return std::move(ret);
}


int Xengine::priority( const token& t ) {
    int p = 0;  //[TODO]添加了好多运算符,这个方法待更新

    /**
     * 所有运算符
     * p越小,优先级越高
     */
    while( true ) {
        if( !t.is(CT::OPERATOR) ) break;
        p += 1;

        if( t.is(VT::MEMBER) )break;
        p += 1;
        //if( t.is(VN::INDEX) ) break;p += 1;                   //单目运算符    后
        if( t.is(VT::INCRESS,VT::DECRESS) ) break;
        p += 1;       //单目运算符    前后
        //if( t.is(VT::prePLUS, VT::preMINUS) ) break; 
        p += 1;    //单目运算符    前
        //if( t.is(VT::ADDRESS,VT::REFER) ) break;
        p += 1;         //单目运算符    前
        if( t.is(VT::NOT) ) break; 
        p += 1;                      //单目运算符    前
        if( t.is(VT::bREV) ) break;
        p += 1;                      //单目运算符    前
        if( t.is(VT::SHL,VT::SHR) ) break;
        p += 1;               //双目运算符    左结合
        if( t.is(VT::bAND) ) break;
        p += 1;                      //双目运算符    左结合
        if( t.is(VT::bXOR) ) break;
        p += 1;
        if( t.is(VT::bOR) ) break;
        p += 1;
        if( t.is(VT::MOL,VT::MUL,VT::DIV) ) break;
        p += 1;
        if( t.is(VT::PLUS,VT::MINUS) ) break;
        p += 1;
        //if( t.is(VT::AS) ) break;p += 1;
        if( t.is(VT::RANGE) ) break;
        p += 1;
        if( t.is(CT::RELATION) ) break;
        p += 1;
        if( t.is(VT::AND) ) break;
        p += 1;
        if( t.is(VT::OR) ) break;
        p += 1;
        if( t.is(CT::ASSIGN) ) break;
        p += 1;
        break;
    }
    return p;
}

}

#endif