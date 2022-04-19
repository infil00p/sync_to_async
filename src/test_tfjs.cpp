#include "doctest/doctest.h"
#include "tfjs.hpp"

TEST_CASE("Import tfjs")
{
    auto status = tfjs::import();
    REQUIRE(status == Utils::JsResultStatus::OK);
}

TEST_CASE("Load model from path")
{

}

TEST_CASE("Load model from buffer")
{

}

TEST_CASE("Predict with loaded model")
{

}
