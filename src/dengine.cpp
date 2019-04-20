#ifndef __dengine_cpp__
#define __dengine_cpp__

#include "dengine.hpp"

#include <fstream>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

namespace alioth {
using namespace std;

string Dengine::vfdm::detectApp(const string& capp) const {
    switch( space & 0xFF00 ) {
        case Work : return capp;
        case Root : return "alioth";
        case Apps : return app;
        default : break;
    }
    return "";
}

bool Dengine::vfdm::operator==(const vfdm& an ) const {
    if( (space & Apps) != 0 ) return name == an.name and app == an.app;
    return name == an.name and space == an.space;
}

bool Dengine::vfdm::operator!=(const vfdm& an) const {
    return !(an == *this);
}

Dengine::Dengine():
mdirRoot("/usr/lib/alioth/"),
mdirWork("."+string(dirdvs)),
mdirArc("arc"+string(dirdvs)),
mdirBin("bin"+string(dirdvs)),
mdirDoc("doc"+string(dirdvs)),
mdirLib("lib"+string(dirdvs)),
mdirInc("inc"+string(dirdvs)),
mdirObj("obj"+string(dirdvs)),
mdirSrc("src"+string(dirdvs)),
midoc(nullptr),
modoc(nullptr),
medoc(nullptr),
msdoc(nullptr){

}

Dengine::~Dengine(){

}

unique_ptr<istream> Dengine::getIs( const string& fname, vspace space, const string& app )const {
    using namespace std;
    if( midoc ) return midoc(fname,space,app);
    string path = getPath(fname,space,app);
    unique_ptr<ifstream> is = make_unique<ifstream>(path);
    if( !is->good() ) return nullptr;
    return is;
}
int Dengine::getIfd( const string& fname, vspace space, const string& app )const {
    using namespace std;
    //if( midoc ) return midoc(fname,space,app);
    string path = getPath(fname,space,app);
    return open(path.data(),O_RDONLY,0644);
}
unique_ptr<ostream> Dengine::getOs( const string& fname, vspace space, const string& app )const {
    using namespace std;
    if( modoc ) return modoc(fname,space,app);
    string path = getPath(fname,space,app);
    unique_ptr<ofstream> os = make_unique<ofstream>(path);
    if( !os->good() ) return nullptr;
    return os;
}
int Dengine::getOfd( const string& fname, vspace space, const string& app )const {
    using namespace std;
    //if( midoc ) return modoc(fname,space,app);
    string path = getPath(fname,space,app);
    return open(path.data(),O_WRONLY|O_CREAT|O_TRUNC,0644);
}
Dengine::vfds Dengine::enumFile( vspace space, const string& subdir, const string& app )const {
    using namespace std;
    if( medoc ) return medoc(space,subdir,app);
    string path = getPath(subdir,space,app);
    vfds pool;
    DIR* dir = opendir(path.data());
    if( !dir ) return pool;
    dirent* p = nullptr;
    struct stat stat;
    while( (p=readdir(dir)) != nullptr ) {
        string name(p->d_name);
        if( name == "." or name == ".." ) continue;
        name = path + name;
        if( ::stat(name.data(), &stat ) ) continue;
        pool << (vfd){
            {name : p->d_name,
            app  : app,
            space: space},
            mtim : stat.st_mtime,
            size : (size_t)stat.st_size
        };
    }
    closedir(dir);
    return pool;


}

bool Dengine::statFile( const string& fname, vspace space, const string& app, vfd& st )const {
    if( msdoc ) return msdoc(fname,space,app,st);
    string path = getPath(fname,space,app);
    struct stat stat;
    if( ::stat(path.data(),&stat) ) return false;
    st.name = fname;
    st.app = app;
    st.space = space;
    st.mtim = stat.st_mtime;
    st.size = stat.st_size;
    return true;
}

string Dengine::getPath( const string& docName, vspace space, const string& app )const {
    string st;
    vspace p = (vspace)(space & 0xFF00);
    space = (vspace)(space & 0x00FF);
    switch( space ) {
        case Arc:st = mdirArc;break;
        case Bin:st = mdirBin;break;
        case Doc:st = mdirDoc;break;
        case Lib:st = mdirLib;break;
        case Inc:st = mdirInc;break;
        case Obj:st = mdirObj;break;
        case Src:st = mdirSrc;break;
        //default : return "";
    }

    if( st[st.size()-1] != '/' ) st += "/";

    switch( p ) {
        case Work: st = mdirWork + st + docName;break;
        case Root: st = mdirRoot + st + docName;break;
        case Apps: st = mdirRoot + "apps" + dirdvs +  app + dirdvs + st + docName;break;
        default: return "";
    }
    return st;
}

uistream Dengine::getIs( const vfdm& fdm )const {
    return getIs( fdm.name, fdm.space, fdm.app );
}

int Dengine::getIfd( const vfdm& fdm )const {
    return getIfd( fdm.name, fdm.space, fdm.app );
}

uostream Dengine::getOs( const vfdm& fdm )const {
    return getOs( fdm.name, fdm.space, fdm.app );
}

int Dengine::getOfd( const vfdm& fdm )const {
    return getOfd( fdm.name, fdm.space, fdm.app );
}

Dengine::vfds Dengine::enumFile( const vfdm& fdm )const {
    return enumFile( fdm.space, fdm.name, fdm.app );
}

string Dengine::getPath( const vfdm& fdm )const {
    return getPath( fdm.name, fdm.space, fdm.app );
}

bool Dengine::setMethodGetIs( IstreamGetter& methodIDoc ) {
    midoc = methodIDoc;
    return true;
}
bool Dengine::setMethodGetOs( OstreamGetter& methodODoc ) {
    modoc = methodODoc;
    return true;
}
bool Dengine::setMethodEnumFile( VfileEnumer& methodEDoc ) {
    medoc = methodEDoc;
    return true;
}
bool Dengine::setMethodStatFile( VfileStater& methodSDoc ) {
    msdoc = methodSDoc;
    return true;
}

bool Dengine::setSpacePath( vspace space, string dir ) {
    if( dir[dir.size()-1] != '/' ) dir += '/';
    if( space == Work ) mdirWork = dir;
    else if( space == Root ) mdirRoot = dir;
    else if( space == Arc ) mdirArc = dir;
    else if( space == Bin ) mdirBin = dir;
    else if( space == Doc ) mdirDoc = dir;
    else if( space == Lib ) mdirLib = dir;
    else if( space == Inc ) mdirInc = dir;
    else if( space == Obj ) mdirObj = dir;
    else if( space == Src ) mdirSrc = dir;
    else return false;
    return true;
}

bool Dengine::config( Jsonz conf ) {
    if( !conf.is(JObject) ) return false;
    if( JString == conf["root-dir"].tell() )
        if( !setSpacePath(Root, conf["root-dir"]) ) return false;
    if( JString == conf["work-dir"].tell() )
        if( setSpacePath(Work, conf["work-dir"])) return false;
    if( JString == conf["arc-dir"].tell() )
        if( setSpacePath(Arc, conf["arc-dir"])) return false;
    if( JString == conf["bin-dir"].tell() )
        if( setSpacePath(Bin, conf["bin-dir"])) return false;
    if( JString == conf["doc-dir"].tell() )
        if( setSpacePath(Doc, conf["doc-dir"])) return false;
    if( JString == conf["lib-dir"].tell() )
        if( setSpacePath(Lib, conf["lib-dir"])) return false;
    if( JString == conf["inc-dir"].tell() )
        if( setSpacePath(Inc, conf["inc-dir"])) return false;
    if( JString == conf["src-dir"].tell() )
        if( setSpacePath(Src, conf["src-dir"])) return false;
    return true;
}

}

#endif