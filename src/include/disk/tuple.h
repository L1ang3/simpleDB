#include<vector>
#include "config/config.h"

namespace spdb{
    class Tuple{
        private:
        char*data_;
        std::vector<Cloum> cloums_;
        RID rid_;

        public:
        auto GetCloumAt(int){}
    };
}