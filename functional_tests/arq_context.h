#pragma once
#include "arq_in_functional_tests.h"
#include <vector>
#include <array>

struct ArqContext
{
    explicit ArqContext(arq_cfg_t const &config);
    ~ArqContext();
    ArqContext() = delete;
    ArqContext(ArqContext const &) = delete;
    ArqContext &operator =(ArqContext const &) = delete;

    arq_t *arq;
    void *seat;
};

