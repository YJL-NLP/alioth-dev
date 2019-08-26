#ifndef __lengine_cpp__
#define __lengine_cpp__

#include "lengine.hpp"
#include <regex>

namespace alioth {
using namespace std;

static bool zero(const void* ptr) {
    return ptr == nullptr;
}

Lengine::logs& Lengine::logr::operator()(const string& p ) {
    insert(logs(p),-1);
    return operator[](-1);
}

Lengine::logs Lengine::fordoc(const string& fname) {
    return move(logs(fname));
}

bool Lengine::config( const Jsonz& conf ) {
    if( conf.test<JBoolean>("color") ) mecolor = (bool)conf.at("color");
    if( conf.test<JBoolean>("path") ) mepath = (bool)conf.at("path");
    if( conf.turn<JObject>("tmpls",[&](const Jsonz& tmpls ){

        auto rpos = regex(R"(n|(b|e)\d+)"); //使用正则表达式判断模式是否满足格式
        tmpls.foreach([&](const string& k, const Jsonz& tmpl) -> bool{
            char* end = nullptr;
            int ki = strtol(k.data(),&end,10);
            if( end and tmpl.test<JInteger>("sev") and tmpl.test<JString>("beg","end","msg") and regex_match((string)tmpl.at("beg"),rpos) and regex_match((string)tmpl.at("end"),rpos) ) {
                auto& tm = mrepo[ki] = (logt){
                    sev: (Severity)(int)tmpl.at("sev"),
                    beg: (string)tmpl.at("beg"),
                    end: (string)tmpl.at("end"),
                    msg: (string)tmpl.at("msg")
                };

                if( tmpl.test<JArray>("sub") ) tmpl.at("sub").foreach([&](const Jsonz& sub ) -> bool {
                    #warning [TODO]:安全性的检查
                    tm.sub.construct(-1);
                    tm.sub[-1].tno = (int)sub.at(0);
                    tm.sub[-1].pat = (int)sub.at(1);
                    for( size_t i = 2; i < sub.count(); i++ ) tm.sub[-1].arg << (int)sub.at(i);
                    return true;
                });
            }
            return true;
        });
    })) return true;
    return false;
}

void Lengine::color( bool c ) {
    mecolor = c;
}

void Lengine::path( bool p ) {
    mepath = p;
}

void Lengine::path( const string& p ) {
    mpath = p;
}

Jsonz Lengine::operator() ( const logr& lr ) {
    Jsonz rep = JArray;

    for( auto& s : lr ) {
        auto r = operator()(s);
        if( r.is(JArray) ) r.foreach([&]( Jsonz& i){
            rep.insert(move(i),-1);
            return true;
        });
    }

    return move(rep);
}

Jsonz Lengine::operator() ( const logs& ls ) {
    Jsonz rep = JArray;

    mpath = ls.path;

    for( auto& i : ls ) {
        auto s = operator()(i);
        rep.insert(move(s),-1);
    }

    return move(rep);
}

Jsonz Lengine::operator() ( const logi& li ) {
    string res;
    int off = 0;
    int state = 1;
    bool stay = false;
    bool colored = false;
    Jsonz ret = JObject;

    if( !mrepo.count(li.tno) ) return JNull;
    auto tmpl = mrepo.at(li.tno);
    
    ret["sev"] = tmpl.sev;

    if( tmpl.beg == "n" ) {
        ret["begl"] = 0;
        ret["begc"] = 0;
    } else {
        long ind = strtol(tmpl.beg.data()+1, 0, 10 );
        auto& arg = li.arg[ind];
        if( zero(&arg) ) return JNull;
        ret["begl"] = (tmpl.beg[0]=='b')?arg.bl:arg.el;
        ret["begc"] = (tmpl.beg[0]=='b')?arg.bc:arg.ec;
    }

    if( tmpl.end == "n" ) {
        ret["endl"] = 0;
        ret["endc"] = 0;
    } else {
        long ind = strtol(tmpl.end.data()+1, 0, 10 );
        auto& arg = li.arg[ind];
        if( zero(&arg) ) return JNull;
        ret["endl"] = (tmpl.end[0]=='b')?arg.bl:arg.el;
        ret["endc"] = (tmpl.end[0]=='b')?arg.bc:arg.ec;
    }

    if( mepath ) {
        //res = mpath + ":";
        if( tmpl.beg != "n" and tmpl.end != "n" ) res = mpath + ":" + to_string((int)ret["begl"]) + ":" + to_string((int)ret["begc"]) + " : ";
        else res = mpath + " : ";
        switch( tmpl.sev ) {
            case 1: res += (mecolor?"\033[1;31merror(":"error(");break;
            case 2: res += (mecolor?"\033[1;35mwarning(":"warning(");break;
            case 3: res += (mecolor?"\033[1;34minformation(":"information(");break;
            case 4: res += (mecolor?"\033[1;36mhint(":"hint(");break;
        }
        res += to_string((int)li.tno) + ")" + (mecolor?"\033[0m:":":");
    }

    while( state > 0 ) {
        switch( auto c = tmpl.msg[off]; state ) {
            case 1:
                if( c == '%' ) state = 2;
                else if( c == '\0' ) state = 0;
                else res += c;
                break;
            case 2:
                if( isdigit(c) ) {stay = true;state=3;break;}
                else if( !mecolor ) {state=3;break;}
                else switch( c ) {
                    case 'r':res += "\033[31m";break;
                    case 'R':res += "\033[1;31m";break;
                    case 'g':res += "\033[32m";break;
                    case 'G':res += "\033[1;32m";break;
                    case 'b':res += "\033[34m";break;
                    case 'B':res += "\033[1;34m";break;
                    case 'y':res += "\033[33m";break;
                    case 'Y':res += "\033[1;33m";break;
                    case 'p':res += "\033[35m";break;
                    case 'P':res += "\033[1;35m";break;
                    case 'c':res += "\033[36m";break;
                    case 'C':res += "\033[1;36m";break;
                    default:return JNull;
                }
                colored = true;
                state = 3;
                break;
            case 3: {
                if( !isdigit(c) ) return JNull;
                auto& arg = li.arg[c-'0'];
                if( zero(&arg) ) return JNull;
                res += arg;
                if( colored ) res += "\033[0m";
                state = 1;
                break;
            }
        }
        if( stay ) stay = false;
        else off += 1;
    }
    ret["msg"] = res;
    ret["pat"] = mpath;

    ret["sub"] = Jsonz(JArray);
    for( auto& sub : tmpl.sub ) {
        mpath = (string)li.arg[sub.pat];
        auto i = logi((Logt)sub.tno);
        for( auto p : sub.arg ) i.arg << li.arg[p];
        ret["sub"].insert(operator()(i),-1);
    }
    for( auto& sub : li.sub ) {
        mpath = get<0>(sub);
        ret["sub"].insert(operator()(get<1>(sub)),-1);
    }
    return ret;
}

}
#endif