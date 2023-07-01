/*
*   Copyright [2023] [Skylark Wireless LLC]
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/


#include <sklk-mii/testing/utils.hpp>

#include "csi_mod.hpp"
#include "loader.hpp"

static bool test_loader()
{
    PRINT_TEST_HEADER();

    auto factory = ref_design_mod_loader_factory();
    auto loader = factory.create(mimo_rrh_scheduler_config{});
    TEST_ASSERT_NOT_NULL(loader.get());

    return true;
}

int main()
{
    test_loader();

    PRINT_SEPARATOR();

    return exit_test();
}
