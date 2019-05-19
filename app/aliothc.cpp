#ifndef __alioth_cpp__
#define __alioth_cpp__

#include "manager.hpp"
#include <sys/stat.h>
#include <sys/stat.h>
#include "xengine.hpp"
#include "yengine.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <string>

using namespace std;
using namespace alioth;

static Manager* pm = nullptr;

int argproc( int argc, char**argv, Manager& manager );
uistream asker(const string& name, vspace space, const string& app);
void writemakefile( string base );
void pagehelp();

int main( int argc, char **argv ) {

    Lengine::logr loggers;
    int cmd = 0;
    auto manager = Manager();
    pm = &manager;
    auto& lengine = manager.getLogEngine();
    auto& dengine = manager.getDocumentEngine();

    cmd = argproc( argc, argv, manager );
    if( cmd <= 0 ) return cmd;
    lengine.config(Jsonz::fromJsonStream(*dengine.getIs("lengine.json",Root)));

    if( cmd == 2 ) { // syntax check
        manager.getLogEngine().color(false);
        manager.Build( Manager::SYNTAXCHECK, loggers );
        auto arr = lengine(loggers);
        Jsonz obj = JObject;
        obj["cmd"] = "diagnostic";
        obj["log"] = arr;
        cout << obj.toJson();
        return 0;
    }

    // normal build
    auto no = manager.Build( Manager::MACHINECODE, loggers );
    if( !no ) cout << "\033[1;31merror\033[0m: build failed !" << endl;

    manager.printModescTable(Work);
    manager.printModescTable(Root);
    manager.printModescTable(Apps);

    auto output = lengine(loggers);
    output.foreach([&](const Jsonz& j){
        cout << (string)j.at("msg") << endl;
        if( j.test<JArray>("sub") ) j.at("sub").foreach([&](const Jsonz& s){cout << "\t" << (string)s.at("msg") << endl; return true;}); 
        else cout << "bad log : " << j.toJson() << endl;
        return true;
    });

    return no?0:1;
}

int argproc( int argc, char **argv, Manager& manager ) {

    string cmd_root = "--root";
    string cmd_R = "-R";
    string cmd_work = "--work";
    string cmd_W = "-W";
    string cmd_V = "-V";
    string cmd_v = "-v";
    string cmd_version = "--version";
    string cmd_app_name = "--app-name";
    string cmd_colon = ":";
    string cmd_init = "--init";
    string cmd_h = "-h";
    string cmd_help = "--help";
    string cmd_semantic_check = "--semantic-check";
    string cmd_ask_input = "--ask-input";

    int ret = 1;

    auto& dengine = manager.getDocumentEngine();

    for( auto i = 1; i < argc; i++ ) {
        if( cmd_semantic_check == argv[i] ) {
            ret = 2; // return for syntax check
        } else if( cmd_ask_input == argv[i] ) {
            dengine.setMethodGetIs(asker);
        } else if( cmd_root == argv[i] or cmd_R == argv[i] ) {
            dengine.setSpacePath(Root,argv[++i]);
        } else if( cmd_work == argv[i] or cmd_W == argv[i] ) {
            dengine.setSpacePath(Work,argv[++i]);
        } else if( cmd_V == argv[i] or cmd_v == argv[i] or cmd_version == argv[i] ) {
            cout << "aliothc " << __aliothc_ver_str__ << " x86_64-pc-linux-gnu\n\toriginal implementation of the Alioth programming language of version " << __alioth_ver_str__ << endl;
            return 0;
        } else if( cmd_colon == argv[i] or cmd_app_name == argv[i] ) {
            if( !manager.setAppName(argv[++i]) ) {
                cout << "\033[1;31merror\033[0m: invalid application name '" << argv[i] <<  "' specified" << endl;
                return -1;
            }
        } else if( cmd_init == argv[i] ) {
            if( !argv[++i] ) {
                cout << "\033[1;31merror\033[0m: dirname missing for initialize progress" << endl;
                return -1;
            }
            string base = argv[i];
            if(base[base.size()-1] != '/' ) base += "/";
            cout << "initialize project structure in dir \033[1;32m" << base << "\033[0m" << endl;
            cout << " making dir " << base; if(  mkdir( base.data(), 0755 ) ) cout << "\033[1;31mfailed\033[0m"; else cout << "\033[1;32msuccess\033[0m"; cout << endl;
            for( auto sub : {"arc","bin","doc","lib","inc","obj","src"} ) {
                cout << " making dir " << base << sub; 
                if( mkdir( (base+sub).data(), 0755 ) ) 
                    cout << " \033[1;31mfailed\033[0m"; 
                else 
                    cout << " \033[1;32msuccess\033[0m"; 
                cout << endl;
            }
            writemakefile(base);
            if( argc != 3 ) cout << "\033[1;36mwarrning\033[0m: other parameters were not used." << endl;
            return 0;
        } else if( cmd_h == argv[i] or cmd_help == argv[i] ) {
            pagehelp();
            return 0;
        } else {
            manager.specifyModule(argv[i]);
        }
    }

    return ret;
}

void writemakefile( string base ) {
    cout << " generating makefile to " << base << "makefile" << endl;
    int mfd = open((base+"makefile").data(), O_CREAT|O_WRONLY|O_TRUNC, 0644 );
    if( auto pos = base.find_last_of('/',base.length()-2); pos == string::npos ) base = string(base.begin(),base.end()-1);
    else base = string(base.begin()+pos+1,base.end()-1);
    string makefile = 
        "SHELL = /bin/bash\n\n"
        "SRC = $(wildcard src/*.alioth)\n"
        "INC = $(wildcard inc/*.alioth)\n"
        "\n"
        "all: " + base + "\n\n" +
        base + ": $(SRC) $(INC)\n"
        "\taliothc : " + base + "\n\n"
        "run: all\n"
        "\t./bin/" + base + "\n\n"
        "clean:\n"
        "\trm -rf bin/* obj/*.o\n"
        "\n"
        ".PHONY: clean\n";
    write( mfd, makefile.data(), makefile.size() );
    close(mfd);
    return;
}

void pagehelp() {
    cout << "Usage : aliothc [\033[1;36mOPTION\033[0m]... [\033[1;34mMODULE\033[0m]... : \033[1;32mAPPLICATION\033[0m\n"
    "  or: aliothc --init \033[1;33mDIR\033[0m\n"
    "Compile \033[1;34mMODULE(s)\033[0m and link them to excutable named bin/\033[1;32mAPPLICATION\033[0m\n"
    "If there were no module specified, compiler compile all modules in the work space.\n"
    "Use the second form to create project structure needed into directory \033[1;33mDIR\033[0m\n"
    "\n"
    "Options listed here:\n"
    "  : <\033[1;32mappname\033[0m>, --app-name <\033[1;32mappname\033[0m>"
        "    specify \033[1;32mname\033[0m for application\n"

    "\n"
    "  -R <\033[1;34mpath\033[0m>, --root <\033[1;34mpath\033[0m>"
        "             specify the \033[1;34mpath\033[0m to alioth root\n"
        "                                       by default, it's set to \033[1;34m'/usr/lib/alioth/'\033[0m\n"

    "\n"
    "  -W <\033[1;34mpath\033[0m>, --work <\033[1;34mpath\033[0m>"
        "             specify the \033[1;34mpath\033[0m to work directory\n"
        "                                       by default, it's set to \033[1;34m'./'\033[0m\n"

    "\n"
    "  -v, -V, --version"
        "                    print the version information of this programm\n"

    "\n"
    "  --init <\033[1;33mdirname\033[0m>"
        "                     construct directory structure needed into the specified \033[1;33mpath\033[0m\n"

    "\n"
    "  -h, --help"
        "                           print this page\n"

    "\n"
    ;
}

uistream asker( const string& name, vspace space, const string& app ) {
    Jsonz ask = JObject;
    ask["cmd"] = "ask for input";
    ask["path"] = pm->getDocumentEngine().getPath(name,space,app);
    auto astr = ask.toJson();
    cout << astr << endl;

    auto answer = ask.fromJsonStream(cin);
    astr = answer.toJson();
    if( answer.is(JString) ) {
        auto stream = std::make_unique<stringstream>();
        stream->str((string)answer);
        return stream;
    } else {
        string path = pm->getDocumentEngine().getPath(name,space,app);
        unique_ptr<ifstream> is = std::make_unique<ifstream>(path);
        if( !is->good() ) return nullptr;
        return is;
    }
}

#endif