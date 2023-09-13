#pragma once

#include <vector>

#include "disk/page.h"
#include "config/config.h"
namespace spdb {
    class TablePage: public Page{
        private:
            page_id_t next_page_id_;
            size_t row_size_;

        public:
    };
}