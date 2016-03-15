#pragma once

#include "nanoarq_functional_test.h"
#include <CppUTest/TestHarness.h>
#include <vector>
#include <array>
#include <cstring>

// expand out the cpputest TEST_GROUP macro so all functional tests can share the 'functional' group.
struct TEST_GROUP_CppUTestGroupfunctional : public Utest {};

