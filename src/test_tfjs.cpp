#include "doctest/doctest.h"
#include "tfjs.hpp"

TEST_CASE("Import tfjs")
{
    auto status = tfjs::import();
    REQUIRE(status == Utils::JsResultStatus::OK);
}
