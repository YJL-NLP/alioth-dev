#ifndef __manager_cpp__
#define __manager_cpp__

#include "manager.hpp"
#include "modesc.hpp"
#include "xengine.hpp"
#include "yengine.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include<fcntl.h>
#include<stdio.h>
#include <wait.h>

namespace alioth {
using namespace std;

static bool isalioth( const string& name ) {
    if( name.size() < 8 ) return false;
    static const string tem = "htoila.";
    auto i = name.end()-1;
    for( auto t : tem ) if( *(i--) != t ) return false;
    return true;
}

static int whichCmd( char* buf, const string& cmd ) {
    int io[2];
    int 
        so = dup(1),
        si = dup(0);
    if( pipe(io) ) return -1;
    dup2(io[1],1);
    dup2(io[0],0);
    auto ret = system( ("which "+cmd).c_str() );
    if( ret == 0 ) scanf("%s",buf);
    dup2(so,1);
    dup2(si,0);
    close(io[0]);
    close(io[1]);
    return ret;
}

void Manager::recordMissing($depdesc D, $modesc M, Lengine::logs& log ) {
    bool already = false;
    for( auto [RD,RM] : missing ) 
        if( (string)D->name == (string)RD->name and D->from(true) == RD->from(true) and M->name == RM->name and M->program == RM->program ) {already=true;break;}
    if( !already ) {
        if( D->from(true).empty() ) log(Lengine::E102,D->name);
        else log(Lengine::E101,D->name,D->from(true));
        missing << (missrec){D,M};
    }
}
void Manager::recordMultipleChoices( string D, $modesc M, Lengine::logs& log ) {
    bool already = false;
    for( auto [RD,RM] : multing ) if( (string)M->name == (string)RM->name and M->program == RM->program and D == RD ) {already=true;break;}
    if( !already ) {
        log(Lengine::E103,D,M->name,M->program);
        multing << (multrec){D,M};
    }
}
void Manager::recordUnreachable( string A, $modesc M, Lengine::logs& log ) {
    bool already = false;
    for( auto [RA,RM] : urching ) if( A == RA and (string)M->name == (string)RM->name and RM->program == RM->program ) {already=true;break;}
    if( !already ) {
        log(Lengine::E104,A,M->name,M->program);
        urching << (urchrec){A,M};
    }
}

bool Manager::verifyDependenceStack( $modesc dep, Lengine::logs& log ) {

    //检查依赖层次
    if( mpadding.size() > 3 ) {
        log(Lengine::E105,mpadding[0]->name,mpadding[0]->program);
        return false;
    }

    //检查循环依赖
    for( auto& pad : mpadding ) {
        if( pad->name == dep->name and pad->program == dep->program ) {
            log(Lengine::E106,dep->name,dep->program);
            return false;
        }
    }
    return true;
}

Manager::Manager(){

}

Manager::~Manager() {
    mwork.clear();
    mroot.clear();
    mapps.clear();
}

bool Manager::setAppName( const string& name ) {
    appname = name;
    return verifyAppName() == 0;
}

string Manager::getAppName() const {
    return appname;
}

int Manager::verifyAppName()const {
    if( appname.empty() ) return 1;
    if( appname == "alioth" ) return 2;
    for( auto& fd : mdengine.enumFile(Root,"apps/") )
        if( fd.name == appname ) return 3;
    return 0;
}

Dengine& Manager::getDocumentEngine() {
    return mdengine;
}

Lengine& Manager::getLogEngine() {
    return mlengine;
}

bool Manager::config( Jsonz conf ) {
    if( conf.is(JObject) ) return false;
    if( JString != conf["version"].tell() ) return false;
    if(  __alioth_ver_str__ != (string)conf["version"] ) return false;
    if( JObject == conf["doc-engine"].tell() )
        if( !mdengine.config( conf["doc-engine"] ) ) return false;
    if( JObject == conf["log-engine"].tell() )
        if( !mlengine.config( conf["log-engine"] ) ) return false;
    return true;
}

bool Manager::loadModescTable( vspace space, const string& app ) {
    
    if( space == Apps and app.empty() ) return loadModescTable(mapps);
    uistream is = mdengine.getIs("mtable.json",space,app);
    if( !is ) return false;
    auto table = Jsonz::fromJsonStream(*is);
    if( !table.is(JObject) ) return false;
    auto& target = (space==Work)?mwork:(space==Root)?mroot:mapps[app];
    return loadModescTable(target,table);
}

bool Manager::loadModescTable( map<string,modescs>& apps ) {
    int count = 0;
    for( auto& app : mdengine.enumFile(Root,"apps/") ) 
        if( loadModescTable(Apps,app.name) ) count ++;
    return count == 0;
}

bool Manager::loadModescTable( modescs& descs, Jsonz table ) {

    if( !table.is(JObject) ) return false;
    if( !table["app-namme"].is(JString) ) return false;
    if( !table["descs"].is(JObject) ) return false;
    if( !table["mtim"].is(JInteger) ) return false;

    descs.aname = (string)table["name"];
    descs.mtim = (int)table["mtim"];
    bool error = false;
    
    table["descs"].foreach([&]( const string& k, Jsonz& v) {

        $modesc ref;

        if( !v["docs"].is(JArray) ) return error = true;
        if( !v["deps"].is(JArray) ) return error = true;

        for( auto& desc : descs )
            if( desc->program == descs.aname and desc->name == k ) {ref = desc;break;}
        if( ref == nullptr ) descs << (ref = new modesc(*this,k,descs.aname));
        
        v["docs"].foreach( [&](Jsonz& doc) {
            Dengine::vfd vfd;
            if( doc.test<JInteger>("space","mtim","size") and doc.test<JString>("name") ) {
                vfd.name = doc["name"];
                vfd.space = (int)doc["space"];
                vfd.mtim = (int)doc["mtim"];
                vfd.size = (int)doc["size"]; 
                ref->appendDocument(vfd);
            } else {
                error = true;
            }
            return true;
        });

        v["deps"].foreach( [&](Jsonz& dep) {
            $depdesc m = new depdesc;
            if( !dep.is(JObject) ) return error = true;
            if( dep["name"].is(JString) ) m->name = token(dep["name"]); else return error = true;
            if( dep["from"].is(JString) ) m->mfrom = token(dep["from"]);
            if( dep["alias"].is(JString) ) m->alias = token(dep["alias"]);
            m->self = ref;
            ref->deps << m;
            return true;
        });

        return true;
    });

    return !error;
}

bool Manager::buildModescTable( Lengine::logr& log, vspace  space, const string& app ) {
    if( space != Work and space != Root and space != Apps ) return false;
    if( space == Apps and app.empty() ) {
        int count = 0;
        for( auto& app : mdengine.enumFile(Root,"apps/") ) 
            if( buildModescTable(log,Apps,app.name) ) count ++;
        return count == 0;
    }
    modescs finally;
    finally.aname = (space==Work)?appname:(space==Root)?"alioth":app;
    finally.mtim = (int)time(nullptr);

    auto build = [&]( Dengine::vfd vfd ) {
        auto is = mdengine.getIs(vfd);
        if( !is ) return;
        Xengine lexical;
        Yengine syntax;

        auto tis = lexical.parseModuleSignature(*is);
        auto logr = mlengine.fordoc(mdengine.getPath(vfd));
        auto st = syntax.detectModuleSignature(tis,logr);
        if( !st ) {
            if( isalioth(vfd.name) ) log << logr;
            return;
        }

        $modesc ref;
        for( auto& mod : finally ) 
            if( mod->name == st->name ) ref = mod;
        if( ref == nullptr ) finally << (ref = new modesc(*this,st->name,vfd.detectApp(appname)));

        for( auto& d : st->deps ) {
            d->self = ref;
            ref->deps << d;
        }
        ref->appendDocument(vfd);
        return;
    };

    bool lastest = true;
    auto& targ = (space==Work)?mwork:(space==Root)?mroot:mapps[app];
    Dengine::vfds todo = mdengine.enumFile(space|Src,"",app);
    for( auto& vfd : mdengine.enumFile(space|Inc,"",app) ) todo << move(vfd);
    for( auto& fd : todo ) if( fd.mtim > targ.mtim ) {lastest = false;break;}

    if( !lastest ) {
        for( auto& td : todo ) build(td);
        targ = finally;
    }

    return true;
}

bool Manager::printModescTable( vspace space, const string& app ) {
    Jsonz table;
    if( !printModescTable(table,space,app) ) return false;
    bool error = false;
    if( space == Work or space == Root or !app.empty() ) {
        auto os = mdengine.getOs("mtable.json",space,app);
        if( !os ) return false;
        *os << table.toJson();
    } else table.foreach([&](const string& aname, Jsonz& t) {
        auto os = mdengine.getOs("mtable.json",Apps,aname);
        if( !os ) return error = true;
        *os << t.toJson();
        return true;
    });
    
    return !error;
}

bool Manager::printModescTable( Jsonz& table, vspace space, const string& app ) {
    if( space != Work and space != Root and space != Apps ) return false;
    table = Jsonz(JObject);
    if( space == Apps and app.empty() ) {
        bool error = false;
        for( auto& fd : mdengine.enumFile(Root,"apps/") )
            if( !printModescTable(table[fd.name],Apps,fd.name) ) error = true;
        return !error;
    }

    modescs* targ = nullptr;
    switch( space ) {
        case Work: 
            targ = &mwork;
            table["name"] = appname;
            break;
        case Root: 
            targ = &mroot;
            table["name"] = "alioth";
            break;
        case Apps: 
            if( mapps.count(app) == 0 ) 
                return false;
            targ = &mapps[app];
            table["name"] = app;
            break;
        default: break;
    }
    

    table["descs"] = Jsonz(JObject);
    for( auto& mod : *targ ) {
        Jsonz& desc = table["descs"][mod->name] =  Jsonz(JObject);
        desc["deps"] = desc["docs"] = Jsonz(JArray);
        
        for(auto& fd : mod->getDocuments(false) ) {
            Jsonz fdesc = JObject;
            fdesc["name"] = fd.name;
            fdesc["space"] = (int)fd.space;
            fdesc["mtim"] = (int)fd.mtim;
            fdesc["size"] = (int)fd.size;
            desc["docs"].insert(move(fdesc),-1);
        }

        for(auto& m : mod->deps ) {
            Jsonz mdesc = JObject;
            mdesc["name"] = m->name;
            mdesc["alias"] = m->alias;
            if( !m->from(false).empty() ) mdesc["from"] = m->from(false);
            desc["deps"].insert(move(mdesc),-1);
        }
    }

    table["mtim"] = (int)time(nullptr);

    return true;
}

void Manager::specifyModule( const string& name ) {
    mnames.push_back(name);
}

bool Manager::Build( const BuildType type, Lengine::logr& log ) {//测试内容

    loadModescTable(Work);
    loadModescTable(Root);
    loadModescTable(Apps);
    buildModescTable(log,Work);
    buildModescTable(log,Root);
    buildModescTable(log,Apps);

    missing.clear();
    multing.clear();
    urching.clear();

    bool bfine = true;
    bool bentry = false;

    if( mnames.size() == 0 ) for( auto desc : mwork ) mnames.push_back(desc->name);

    modescs descs;
    for( auto& mname : mnames ) {
        auto res = completDependencies(mname,log,descs);
        if( res < 0 ) bfine = false;
    }
    for( auto& desc : descs ) if( desc->constructAbstractSyntaxTree( log ) ) {
        if( bfine ) 
            if( 2 == msengine.loadModuleDefinition(desc) ) 
                bentry = true;
    } else {
        bfine = false;
    }

    if( !bfine ) return false;

    if( !msengine.performDefinitionSemanticValidation() ) {
        log += msengine.getLog();
        return false;
    }

    vector<string> args;
    char cmd[128];
    if( type != SYNTAXCHECK and appname.size() ) {
        if( bentry ) {
            args.push_back("ld");
            args.push_back("-o");
            args.push_back(mdengine.getPath("",Work|Bin) + appname);
            args.push_back(mdengine.getPath("alioth.o", Root|Obj));
            if( whichCmd(cmd,"ld") ) {cout << "cannot find linker \"ld\"" << endl;bfine = false;}
        } else {
            args.push_back("ar");
            args.push_back("-rcs");
            args.push_back(mdengine.getPath("",Work|Arc) + "lib" + appname + ".a");
            if( whichCmd(cmd,"ar") ) {cout << "cannot find command \"ar\"" << endl;bfine = false;}
        }
    }

    for( auto desc : descs ) {
        
        Dengine::vfdm fd;
        fd.app = desc->program;
        fd.name = desc->name+".o";
        fd.space = Obj;
        fd.space |= desc->program == this->appname?Work:desc->program == "alioth"?Root:Apps;
        args.push_back(mdengine.getPath(fd));

        for( auto imname = mnames.begin(); imname != mnames.end(); imname++  ) if( auto& mname = *imname; mname == desc->name ) {
            mnames.erase(imname);
            auto unit = msengine.performImplementationSemanticValidation(desc,mdengine);
            if( !unit ) bfine = false;
            else if( type != SYNTAXCHECK )
                if( !msengine.triggerBackendTranslation(unit, fd, mdengine) ) 
                    bfine = false;
            break;
        }
    }

    log += msengine.getLog();
    if( type != SYNTAXCHECK and appname.size() and bfine and descs.size() and fork() == 0 ) {
        vector<const char*> sargs;
        for( auto& arg : args ) sargs.push_back(arg.c_str()); sargs.push_back(nullptr);
        execv( cmd, (char*const*)&sargs[0] );
    }
    int st;
    wait(&st);
    return bfine and st == 0;
}

int Manager::completDependencies( const string& name, Lengine::logr& log, modescs& output ) {
    int count = -1;
    auto& l = log.construct(-1,"[Building '"+name+"' from '"+appname+"']");

    for( auto& w : mwork ) if( bool found = false; w->name == name ) {
        for( auto& m : output ) if( m->program == w->program and m->name == w->name ) {found = true;break;}
        if( !found ) output << w;
        count = 0;
    }
    if( count < 0 ) return l(Lengine::E101,name,appname),-1;

    for(  auto it = output.end()-1; it != output.end(); it++ ) {
        mpadding.clear();
        if( completDependencies(*it,log,output) ) count++;
    }

    return count;
}

int Manager::closeUpDependency( $depdesc dep ) {
    $modesc ref = nullptr;
    string dfrom = dep->from(true);
    auto self = ($modesc)dep->self;
    auto& local = self->program==appname?mwork:self->program=="alioth"?mroot:mapps[self->program];

    if( dfrom.empty() or dfrom == self->program ) for( auto& mod : local )
        if( mod->name == (string)dep->name ) {ref = mod;break;}
    if( ref == nullptr and (dfrom.empty() or dfrom == "alioth") ) for( auto& mod : mroot )
        if( mod->name == (string)dep->name ) {ref = mod;break;}
    if( ref == nullptr and dfrom != "alioth" ) {
        if( bool error = false; dfrom.empty() ) for( auto& [k,v] : mapps ) {
            for( auto& mod : v ) if( mod->name == (string)dep->name ) {
                if( ref == nullptr ) {
                    ref = mod;
                    break;
                } else {
                    ref = nullptr;
                    error = true;
                    return 3;//recordMultipleChoices(dep->name,self,l);
                    break;
                }
            }
            if( error ) break;
        } else if( mapps.count(dfrom) == 0 ) {
            return 2;//recordUnreachable(dfrom,self,l);
        } else for( auto& mod : mapps[dfrom] ) {
            if( mod->name  == (string)dep->name ) {ref = mod;break;}
        }
    }
    if( ref == nullptr ) return 1;//recordMissing(dep,self,l);

    dep->dest = ref;   //闭合依赖关系
    return 0;
}

int Manager::completDependencies( $modesc desc, Lengine::logr& log, modescs& output ) {
    auto& l = log.construct(-1,desc->name+"@"+desc->program);
    if( !verifyDependenceStack(desc,l) ) return false;
    int count = 0;
    
    mpadding << desc;

    for( auto& dep : desc->deps ) {

        switch( closeUpDependency(dep) ) {
            case 1: recordMissing(dep,desc,l);continue;break;
            case 2: recordUnreachable(dep->from(true),desc,l);continue;break;
            case 3: recordMultipleChoices(dep->name,desc,l);continue;break;
            default: break;
        }

        bool found = false;
        if( completDependencies(dep->dest,log,output) != 0 ) 
            count += 1;
        else for( auto& ar : output ) 
            if( ar->program == dep->dest->program and ar->name == dep->dest->name ) {
                found = true;
                break;
            }
        if( !found ) output << dep->dest;
    }

    mpadding.pop();
    return count;
}

}
#endif