#ifndef __jsonz_cpp__
#define __jsonz_cpp__

#include "jsonz.hpp"

#include <map>


static const char TAG_NULL   = 0;
static const char TAG_FALSE  = 1;
static const char TAG_TRUE   = 2;
static const char TAG_INT    = 3;
static const char TAG_STR    = 4;
static const char TAG_REAL   = 5;
static const char TAG_OBJ    = 6;
static const char TAG_ARR    = 7;
static const char END_OBJ    = 8;
static const char END_ARR    = 9;

using jarray = chainz<Jsonz>;
using jobject = std::map<std::string,Jsonz>;

Jsonz::Jsonz( JType type ):mtype(type) {
    switch( type ) {
        case JArray:
            mdata = new jarray;break;
        case JBoolean:
            *(bool*)&mdata = false;break;
        case JInteger:
            *(int*)&mdata = 0;break;
        case JObject:
            mdata = new jobject;break;
        case JReal:
            mdata = new double(0);break;
        case JString:
            mdata = new std::string;break;
        default:
            mdata = nullptr;
    }
}

Jsonz::Jsonz( const std::string& str ):Jsonz(JString) {
    *(std::string*)mdata = str;
}

Jsonz::Jsonz( const Jsonz& an ):mtype(an.mtype) {
    switch( an.mtype ) {
        case JArray:
            mdata = new jarray(*(jarray*)an.mdata);break;
        case JBoolean:
        case JInteger:
            mdata = an.mdata;break;
        case JObject:
            mdata = new jobject(*(jobject*)an.mdata);break;
        case JReal:
            mdata = new double(*(double*)an.mdata);break;
        case JString:
            mdata = new std::string(*(std::string*)an.mdata);break;
        default:
            mdata = nullptr;
    }
}

Jsonz::Jsonz( Jsonz&& an ):mtype(an.mtype),mdata(an.mdata) {
    an.mdata = nullptr;
    an.mtype = JNull;
}

Jsonz::~Jsonz() {
    clear();
}

void Jsonz::clear() {
    switch( mtype ) {
        case JArray:
            delete (jarray*)mdata;break;
        case JObject:
            delete (jobject*)mdata;break;
        case JReal:
            delete (double*)mdata;break;
        case JString:
            delete (std::string*)mdata;break;
        default:break;
    }
    mdata = nullptr;
    mtype = JNull;
}

Jsonz Jsonz::fromJsonStream( std::istream& is, int* endp ) {
    Jsonz ret;
    int pre;    //preview
    int state = 1;
    std::string u;

    auto u2u = [&]() {
        char h,l;
                        
        if( u[0] >= 'a' and u[0] <= 'f' ) h = u[0] - 'a' + 0x0a;
        else h = u[0] - '0';
        h <<= 4;

        if( u[1] >= 'a' and u[0] <= 'f' ) h |= u[1] - 'a' + 0x0a;
        else h |= u[1] - '0';

        if( u[2] >= 'a' and u[2] <= 'f' ) l = u[2] - 'a' + 0x0a;
        else l = u[2] - '0';
        l <<= 4;

        if( u[3] >= 'a' and u[3] <= 'f' ) l |= u[3] - 'a' + 0x0a;
        else l |= u[3] - '0';

        *(std::string*)ret.mdata += l;
        if( h != 0 ) *(std::string*)ret.mdata += h;
    };

    for( pre = is.peek(); state > 0; pre = is.peek() ) 
        switch( pre ) {
            case '\0':case EOF:
                if( state == 1 ) {
                    state = -1;
                } else if( state == 16 ) {
                    state = 0;
                    ret.mtype = JInteger;
                    *(int*)&ret.mdata = strtol(u.data(),nullptr,10);
                } else if( state == 18 or state == 21 ) {
                    state = 0;
                    ret.mtype = JReal;
                    ret.mdata = new double(strtod(u.data(),nullptr));
                } else {
                    state = -1;
                } break;
            case ' ':case '\t':case '\n':case '\r':
                if( state == 1 ) {
                    is.get();
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 ) {
                    state = 0;
                    ret.mtype = JInteger;
                    *(int*)&ret.mdata = strtol(u.data(),nullptr,10);
                } else if( state == 18 or state == 21 ) {
                    state = 0;
                    ret.mtype = JReal;
                    ret.mdata = new double(strtod(u.data(),nullptr));
                } else if( state == 22 or state == 23 or state == 24 or state == 25 or state == 26 ) {
                    is.get();
                } else {
                    state = -1;
                } break;
            case '\"':
                if( state == 1 ) {
                    is.get();
                    state = 2;
                    ret.mtype = JString;
                    ret.mdata = new std::string();
                } else if( state == 2 ) {
                    is.get();
                    state = 0;
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 22 ) {
                    Jsonz k = fromJsonStream(is);
                    if( !k.is(JString) ) state = -1;
                    else { u = (std::string)k;state = 23;}
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case '\\':
                if( state == 2 ) {
                    is.get();
                    state = 3;
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += is.get();
                } else {
                    state = -1;
                } break;
            case 'a':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += '\a';
                    is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else if( state == 8 ) {
                    is.get();
                    state = 9;
                } else {
                    state = -1;
                } break;
            case 'b':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += '\b';
                    is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else {
                    state = -1;
                } break;
            case 'c':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else {
                    state = -1;
                } break;
            case 'd':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else  if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else {
                    state = -1;
                } break;
            case 'e':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += '\e';
                    is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else if( state == 7 ) {
                    is.get();
                    *(bool*)&ret.mdata = true;
                    state = 0;
                } else if( state == 11 ) {
                    is.get();
                    *(bool*)&ret.mdata = false;
                    state = 0;
                } else if( state == 16 or state == 18 ) {
                    u += is.get();
                    state = 19;
                } else {
                    state = -1;
                } break;
            case 'E':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 or state == 18 ) {
                    u += is.get();
                    state = 19;
                } else {
                    state = -1;
                }break;
            case 'f':
                if( state == 1 ) {
                    is.get();
                    state = 8;
                    ret.mtype = JBoolean;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case 'l':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 9 ) {
                    is.get();
                    state = 10;
                } else if( state == 13 ) {
                    is.get();
                    state = 14;
                } else if( state == 14 ) {
                    is.get();
                    state = 0;
                    ret.mdata = nullptr;
                } else {
                    state = -1;
                } break;
            case 'n':
                if( state == 1 ) {
                    is.get();
                    state = 12;
                    ret.mtype = JNull;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += '\n';
                    is.get();
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case 'o':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else {
                    state = -1;
                } break;
            case 'r':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    is.get();
                    state = 2;
                    *(std::string*)ret.mdata += '\r';
                } else if( state == 5 ) {
                    is.get();
                    state = 6;
                } else {
                    state = -1;
                } break;
            case 's':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 10 ) {
                    is.get();
                    state = 11;
                } else {
                    state = -1;
                } break;
            case 't':
                if( state == 1 ) {
                    is.get();
                    state = 5;
                    ret.mtype = JBoolean;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    state = 2;
                    *(std::string*)ret.mdata += '\t';
                    is.get();
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case 'u':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 3 ) {
                    is.get();
                    state = 4;
                    u.clear();
                } else if( state == 6 ) {
                    is.get();
                    state = 7;
                } else if( state == 12 ) {
                    is.get();
                    state = 13;
                } else {
                    state = -1;
                } break;
            case '0' ... '9':
                if( state == 1 ) {
                    u = is.get();
                    state = 16;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 4 ) {
                    u += is.get();
                    if( u.size() == 4 ) {
                        u2u();
                        state = 2;
                    }
                } else if( state == 15 ) {
                    u += is.get();
                    state = 16;
                } else if( state == 16 ) {
                    u += is.get();
                } else if( state == 17 ) {
                    u += is.get();
                    state = 18;
                } else if( state == 18 ) {
                    u += is.get();
                } else if( state == 19 or state == 20 ) {
                    u += is.get();
                    state = 21;
                } else if( state == 21 ) {
                    u += is.get();
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case '.':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 ) {
                    u += is.get();
                    state = 17;
                } else {
                    state = -1;
                } break;
            case '-':
                if( state == 1 ) {
                    u = is.get();
                    state = 15;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 19 ) {
                    u += is.get();
                    state = 20;
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case '+':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 19 ) {
                    u += is.get();
                    state = 20;
                } else {
                    state = -1;
                } break;
            case '{':
                if( state == 1 ) {
                    is.get();
                    state = 22;
                    ret.mtype = JObject;
                    ret.mdata = new jobject;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                }break;
            case '}':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 ) {
                    state = 0;
                    ret.mtype = JInteger;
                    *(int*)&ret.mdata = strtol(u.data(),nullptr,10);
                } else if( state == 18 or state == 21 ) {
                    state = 0;
                    ret.mtype = JReal;
                    ret.mdata = new double(strtod(u.data(),nullptr));
                } else if( state == 22 or state == 24 ) {
                    is.get();
                    state = 0;
                } else {
                    state = -1;
                } break;
            case '[':
                if( state == 1 ) {
                    is.get();
                    state = 25;
                    ret.mtype = JArray;
                    ret.mdata = new jarray;
                } else if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 25 ) {
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                }break;
            case ']':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 ) {
                    state = 0;
                    ret.mtype = JInteger;
                    *(int*)&ret.mdata = strtol(u.data(),nullptr,10);
                } else if( state == 18 or state == 21 ) {
                    state = 0;
                    ret.mtype = JReal;
                    ret.mdata = new double(strtod(u.data(),nullptr));
                } else if( state == 25 or state == 26 ) {
                    is.get();
                    state = 0;
                } else {
                    state = -1;
                } break;
            case ',':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 16 ) {
                    state = 0;
                    ret.mtype = JInteger;
                    *(int*)&ret.mdata = strtol(u.data(),nullptr,10);
                } else if( state == 18 or state == 21 ) {
                    state = 0;
                    ret.mtype = JReal;
                    ret.mdata = new double(strtod(u.data(),nullptr));
                } else if( state == 24 ) {
                    is.get();
                    Jsonz k = fromJsonStream(is);
                    if( !k.is(JString) ) {
                        state = -1;
                    } else {
                        u = (std::string)k;
                        state = 23;
                    }
                } else if( state == 26 ) {
                    is.get();
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret.insert( std::move(v), -1 );
                        state = 26;
                    }
                } else {
                    state = -1;
                } break;
            case ':':
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else if( state == 23 ) {
                    is.get();
                    Jsonz v = fromJsonStream(is,&state);
                    if( state == 0 ) {
                        ret[u] = std::move(v);
                        state = 24;
                    }
                } else {
                    state = -1;
                }break;
            default:
                if( state == 2 ) {
                    *(std::string*)ret.mdata += is.get();
                } else {
                    state = -1;
                }break;
        }

    if( endp ) *endp = state;

    if( state != 0 )
        return Jsonz(JNull);
    return ret;
}

Jsonz Jsonz::fromAbonStream( std::istream& is, int* endp ) {
    Jsonz ret;
    int state = 1;
    
    switch( is.peek() ) {
        case TAG_NULL:
            is.get();
            state = 0;
            break;
        case TAG_TRUE:
            is.get();
            state = 0;
            ret = true;
            break;
        case TAG_FALSE:
            is.get();
            state = 0;
            ret = false;
            break;
        case TAG_INT: {
            is.get();
            ret.mtype = JInteger;
            ret.mdata = 0x00;
            for( size_t i = 0; i < sizeof(int); i++ )
                if( is.peek() == EOF ) {
                    state = -1;
                    break;
                } else {
                    *(int*)&ret.mdata |= (is.get()&0x0FF) << (i<<3);
                }
            if( state > 0 )
                state = 0;
        } break;
        case TAG_REAL: {
            is.get();
            ret.mtype = JReal;
            ret.mdata = new double;
            for( size_t i = 0; i < sizeof(double); i++ )
                if( is.peek() == EOF ) {
                    state = -1;
                    break;
                } else {
                    *(int*)ret.mdata |= (is.get()&0x0FF) << (i<<3);
                }
            if( state > 0 )
                state = 0;
        } break;
        case TAG_STR: {
            is.get();
            ret.mtype = JString;
            ret.mdata = new std::string;
            while( is.peek() != '\0' )
                if( is.peek() == EOF ) {
                    state = -1;
                    break;
                } else {
                    *(std::string*)ret.mdata += is.get();
                }
            is.get();
            if( state > 0 )
                state = 0;
        } break;
        case TAG_ARR: {
            is.get();
            ret.mtype = JArray;
            ret.mdata = new jarray;
            while( is.peek() != END_ARR ) {
                Jsonz v = fromAbonStream(is,&state);
                if( state != 0 ) break;
                ((jarray*)ret.mdata)->push(std::move(v));
            }
            if( state == 0 )
                is.get();
        } break;
        case TAG_OBJ: {
            is.get();
            ret.mtype = JObject;
            ret.mdata = new jobject;
            while( is.peek() != END_OBJ ) {
                Jsonz k = fromAbonStream(is,&state);
                if( !k.is(JString) ) {
                    state = -1;
                    break;
                }
                Jsonz v = fromAbonStream(is,&state);
                if( state != 0 ) break;
                (*(jobject*)ret.mdata)[k] = std::move(v);
            }
            if( state == 0 )
                is.get();
        } break;
    }

    if( endp ) *endp = state;
    if( state != 0 ) return Jsonz(JNull);
    return ret;
}

JType Jsonz::tell()const {
    return mtype;
}
bool Jsonz::is( JType type )const {
    return mtype == type;
}

Jsonz::operator int()const {
    if( mtype == JInteger ) return *(const int*)&mdata;
    return 0;
}
Jsonz::operator double()const {
    if( mtype == JReal ) return *(const double*)mdata;
    return 0;
}
Jsonz::operator bool()const {
    if( mtype == JBoolean ) return *(const bool*)&mdata;
    return false;
}
Jsonz::operator std::string()const {
    if( mtype == JString ) return *(const std::string*)mdata;
    else return "";
}

Jsonz& Jsonz::operator=(const Jsonz& an ) {
    Jsonz n(an);
    clear();
    mtype = n.mtype;
    mdata = n.mdata;
    n.mtype = JNull;
    n.mdata = nullptr;
    return *this;
}
Jsonz& Jsonz::operator=(Jsonz&& an ) {
    JType nt = an.mtype;
    void* nd = an.mdata;
    an.mtype = JNull;
    an.mdata = nullptr;
    clear();
    mtype = nt;
    mdata = nd;
    return *this;
}
Jsonz& Jsonz::operator=(bool v) {
    clear();
    mtype = JBoolean;
    *(bool*)&mdata = v;
    return *this;
}
Jsonz& Jsonz::operator=(int v) {
    clear();
    mtype = JInteger;
    *(int*)&mdata = v;
    return *this;
}
Jsonz& Jsonz::operator=(double v) {
    clear();
    mtype = JReal;
    mdata = new double(v);
    return *this;
}
Jsonz& Jsonz::operator=(const std::string& v) {
    clear();
    mtype = JString;
    mdata = new std::string(v);
    return *this;
}
Jsonz& Jsonz::operator=(const char* v ) {
    clear();
    mtype = JString;
    mdata = new std::string(v);
    return *this;
}

bool Jsonz::insert( const Jsonz& v, int index ) {
    if( mtype != JArray ) return false;
    if( index < 0 ) index = count() + 1 + index;
    if( index < 0 ) {
        while( index++ != 0 ) (*(jarray*)mdata).insert(Jsonz(JNull),0);
        (*(jarray*)mdata).insert(v,0);
    } else {
        while( (long long)count() < index ) (*(jarray*)mdata).insert(Jsonz(JNull),-1);
        (*(jarray*)mdata).insert(v,index);
    }
    return true;
}
bool Jsonz::insert( Jsonz&& v, int index ) {
    if( mtype != JArray ) return false;
    if( index < 0 ) index = count() + 1 + index;
    if( index < 0 ) {
        while( index++ != 0 ) (*(jarray*)mdata).insert(Jsonz(JNull),0);
        (*(jarray*)mdata).insert(v,0);
    } else {
        while( (long long)count() < index ) (*(jarray*)mdata).insert(Jsonz(JNull),-1);
        (*(jarray*)mdata).insert(std::move(v),index);
    }
    return true;
}
size_t Jsonz::count() const {
    if( mtype == JArray ) return ((const jarray*)mdata)->size();
    else if( mtype == JObject ) return ((const jobject*)mdata)->size();
    return 0;
}

size_t Jsonz::count( const std::string& key ) const {
    if( mtype != JObject ) return 0;
    return ((const jobject*)mdata)->count(key);
}
Jsonz& Jsonz::operator[](const std::string& key) {
    if( mtype != JObject ) return *(Jsonz*)0x0;
    return (*(jobject*)mdata)[key];
}
const Jsonz& Jsonz::at(const std::string& key)const {
    if( mtype == JObject and count(key) )
        return (*(const jobject*)mdata).at(key);
    else
        return *(const Jsonz*)0x0;
}
Jsonz& Jsonz::operator[]( int index ) {
    if( mtype != JArray ) return *(Jsonz*)0x0;
    jarray& arr = *(jarray*)mdata;
    if( index >= arr.size() or index < -arr.size() ) insert(Jsonz(JNull),index);
    return arr[index];
}
const Jsonz& Jsonz::at( int index )const {
    if( mtype != JArray ) return *(Jsonz*)0x0;
    const jarray& arr = *(const jarray*)mdata;
    return arr[index];
}

int Jsonz::foreach( std::function<bool(Jsonz&)> fun ) {
    if( mtype != JArray ) return -1;
    int i = 0;
    for( auto& v : *(jarray*)mdata ) 
        if( !fun(v) ) break;
        else i += 1;
    return i;
}
int Jsonz::foreach( std::function<bool(const Jsonz&)> fun )const {
    if( mtype != JArray ) return -1;
    int i = 0;
    for( auto& v : *(const jarray*)mdata ) 
        if( !fun(v) ) break;
        else i += 1;
    return i;
}
int Jsonz::foreach( std::function<bool(const std::string&,Jsonz&)> fun ) {
    if( mtype != JObject ) return -1;
    int i = 0;
    for( auto& p : *(jobject*)mdata ) 
        if( !fun(p.first,p.second) ) break;
        else i += 1;
    return i;
}
int Jsonz::foreach( std::function<bool(const std::string&,const Jsonz&)> fun )const {
    if( mtype != JObject ) return -1;
    int i = 0;
    for( auto& p : *(const jobject*)mdata ) 
        if( !fun(p.first,p.second) ) break;
        else i += 1;
    return i;
}

bool Jsonz::drop( int index ) {
    if( mtype == JArray )
        return ((jarray*)mdata)->remove(index);
    return false;
}
bool Jsonz::drop( const std::string& key ) {
    if( mtype == JObject )
        return ((jobject*)mdata)->erase(key) > 0;
    return false;
}

std::string Jsonz::toJson() const {
    std::string ret;
    switch( mtype ) {
        case JNull : ret = "null";break;
        case JBoolean : ret = (*(const bool*)&mdata)?"true":"false";break;
        case JInteger: ret = std::to_string(*(const int*)&mdata);break;
        case JReal: ret = std::to_string(*(const double*)mdata);break;
        case JString: {
            ret = "\"";
            const std::string& v = *(const std::string*)mdata;
            for( auto& c : v ) {
                switch( c ) {
                    case '\a':ret += "\\a";break;
                    case '\b':ret += "\\b";break;
                    case '\n':ret += "\\n";break;
                    case '\r':ret += "\\r";break;
                    case '\t':ret += "\\t";break;
                    case '\\':ret += "\\\\";break;
                    case '\"':ret += "\\\"";break;
                    default: 
                        if( iscntrl(c) ) {
                            char buf[3];
                            sprintf(buf,"%02x",c);
                            ret += "\\u00" + std::string(buf);
                        } else {
                            ret += c;
                        }break;
                }
            }
            ret += "\"";
        }break;
        case JObject: {
            const jobject& obj = *(jobject*)mdata;
            size_t count = obj.size();
            ret = "{";
            for( auto& p : obj ) {
                Jsonz k;
                k = p.first;
                ret += k.toJson() + ":" + p.second.toJson();
                if( count-- > 1 ) ret += ",";
            }
            ret += "}";
        }break;
        case JArray: {
            const jarray& arr = *(jarray*)mdata;
            size_t count = arr.size();
            ret = "[";
            for( auto& v : arr ) {
                ret += v.toJson();
                if( count-- > 1 ) ret += ",";
            }
            ret += "]";
        }break;
    }
    return ret;
}

abon Jsonz::toAbon() const {
    abon ret;
    switch( mtype ) {
        case JNull: ret.push_back(TAG_NULL);break;
        case JBoolean: ret.push_back((*(const bool*)&mdata)?TAG_TRUE:TAG_FALSE);break;
        case JInteger:
            ret.push_back(TAG_INT);
            for( size_t i = 0; i < sizeof(int); i++ )
                ret.push_back(i[(const char*)&mdata]);
            break;
        case JReal:
            ret.push_back(TAG_REAL);
            for( size_t i = 0; i < sizeof(double); i++ )
                ret.push_back(i[(const char*)&mdata]);
            break;
        case JArray:
            ret.push_back(TAG_ARR);
            for( auto& i : *(const jarray*)mdata ) {
                auto t = i.toAbon();
                ret.insert(ret.end(),t.begin(),t.end());
            }
            ret.push_back(END_ARR);
            break;
        case JString:
            ret.push_back(TAG_STR);
            for( char c : *(const std::string*)mdata ) ret.push_back(c);
            ret.push_back(0);
            break;
        case JObject:
            ret.push_back(TAG_OBJ);
            for( auto& p : *(const jobject*)mdata ) {
                ret.push_back(TAG_STR);
                for( char c : p.first ) ret.push_back(c);
                ret.push_back(0);
                auto t = p.second.toAbon();
                ret.insert(ret.end(),t.begin(),t.end());
            }
            ret.push_back(END_OBJ);
            break;
    }
    return std::move(ret);
}

#endif