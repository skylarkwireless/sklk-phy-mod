#include <sklk-cpptest.hpp>

#include "loader.hpp"

TEST(TestRefDesignLoader, TestLoader)
{
    auto factory = ref_design_mod_loader_factory();
    auto loader = factory.create(mimo_rrh_scheduler_config{});
    EXPECT_FALSE(sklkcpptest::pointer_null(loader));
}
