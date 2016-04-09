#pragma once
#include "arq_in_functional_tests.h"
#include "arq_context.h"
#include <CppUTest/TestHarness.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cstring>
#include <cstdio>

// expand out the cpputest TEST_GROUP macro so all functional tests can share the 'functional' group.
struct TEST_GROUP_CppUTestGroupfunctional : public Utest {};

