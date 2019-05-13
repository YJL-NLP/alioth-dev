#include "manager.hpp"
#include "xengine.hpp"
#include "yengine.hpp"
#include <iostream>
#include <sstream>

int main( int argc, char** argv ) {
    using namespace alioth;
    using namespace std;

    Manager global = Manager();
    global.setAppName("global");
    auto& leng = global.getLogEngine();
    auto& deng = global.getDocumentEngine();
    auto logs = leng.fordoc("");
    leng.config(Jsonz::fromJsonStream(*deng.getIs("lengine.json",Root)));
    leng.color(false);
    leng.path(false);

    auto cmd = Jsonz::fromJsonStream(cin);
    Xengine lexic;
    Yengine syntax;
    if( !cmd.test<JString>("cmd") ) return -1;

    if( (string)cmd["cmd"] == "validate" ) {
        if( !cmd.test<JString>("content") ) return -2;
        auto is = stringstream((string)cmd["content"]);
        auto ts = lexic.parseSourceCode(is);
        syntax.constructSyntaxTree(ts,logs);
        cout << leng(logs).toJson();
        return 0;
    }

    return 0;
}