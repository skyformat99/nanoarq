#pragma once

#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>
#include <vector>
#include <array>
#include <cstring>

// expand out the cpputest TEST_GROUP macro so all functional tests can share the 'functional' group.
struct TEST_GROUP_CppUTestGroupfunctional : public Utest {};

