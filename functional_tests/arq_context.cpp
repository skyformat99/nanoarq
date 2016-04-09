#include "functional_tests.h"

ArqContext::ArqContext(arq_cfg_t const &config)
{
    unsigned size;
    {
        arq_err_t const e = arq_required_size(&config, &size);
        CHECK(ARQ_SUCCEEDED(e));
    }

    this->seat = std::malloc(size);
    {
        arq_err_t const e = arq_init(&config, seat, size, &this->arq);
        CHECK(ARQ_SUCCEEDED(e));
    }
}

ArqContext::~ArqContext()
{
    std::free(this->seat);
}

