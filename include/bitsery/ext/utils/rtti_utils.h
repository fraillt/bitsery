//
// Created by fraillt on 17.11.30.
//

#ifndef BITSERY_RTTI_UTILS_H
#define BITSERY_RTTI_UTILS_H

#include <typeinfo>

namespace bitsery {
    namespace ext {
        namespace utils {
            struct StandardRTTI {
                template<typename T>
                static size_t get() {
                    return typeid(T).hash_code();
                }

                template<typename T>
                static size_t get(T &&obj) {
                    return typeid(obj).hash_code();
                }
            };
        }
    }
}

#endif //BITSERY_RTTI_UTILS_H
