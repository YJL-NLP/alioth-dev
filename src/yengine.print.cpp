#ifndef __print_cpp__
#define __print_cpp__

#include "yengine.hpp"

namespace alioth {

Jsonz Yengine::printSyntaxStructure( anything node ) {
    Jsonz json = JObject;
    
    if( auto structure = (agent<nameuc>)node; structure ) {
        json["syntax"] = "nameuc";
        json["content"] = Jsonz(JArray);
        for( auto& atom : structure->msequence ) {
            json["content"].insert(printSyntaxStructure((anything)&atom), -1 );
        }

    } else if( auto structure = (agent<nameuc::atom>)node; structure ) {

    } else if( auto structure = ($typeuc)node; structure ) {

    } else if( auto structure = ($eproto)node; structure ) {

    }
}

}

#endif