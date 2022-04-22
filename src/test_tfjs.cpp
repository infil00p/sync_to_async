#include "doctest/doctest.h"
#include "js_includes.hpp"
#include "proxying_sync_to_async.hpp"
#include "js_includes.hpp"
#include "tfjs.hpp"
#include <iostream>
#include <sstream>
#include <thread>

TEST_CASE("Call to javascript")
{
    SUBCASE("Single call")
    {
        auto executionStatus =
                Utils::queued_js_executor(log_data, "Hello World!");
        REQUIRE(executionStatus == 0);
    }

    SUBCASE("Call with multiple arguments")
    {
        auto executionStatus =
                Utils::queued_js_executor(log_multiple, "Hello World!", "Hello World again!");
        REQUIRE(executionStatus == 0);
    }
    SUBCASE("Multiple calls")
    {
        for (auto i = 0; i < 100; ++i)
        {

            auto executionStatus = Utils::queued_js_executor(log_data, "Hello World Multiple");
//            auto executionStatus = Utils::queued_js_executor(
//                    log_data, "Hello World Multiple");
            REQUIRE(
                    executionStatus == 0);
        }
    }
}

TEST_CASE("Calling a Js function that returns error")
{
    auto executionResult = Utils::queued_js_executor(error_func);
    REQUIRE(executionResult == 1);
}

TEST_CASE("Calling a long running Js function")
{
    auto delay = std::chrono::milliseconds(1000);
    auto t1    = std::chrono::steady_clock::now();
    auto executionResult =
            Utils::queued_js_executor(long_running_func, delay.count());
    auto t2 = std::chrono::steady_clock::now();
    REQUIRE((t2 - t1) >= delay);
    REQUIRE(executionResult == 0);
}

TEST_CASE("Calling a longer running Js function")
{
    // Normally we would probably will have to rethink anything that takes this
    // long but TFJs predict takes around this much in GPU during first run So
    // it's better to test such long delays
    auto delay = std::chrono::milliseconds(10000);
    auto t1    = std::chrono::steady_clock::now();
    auto executionResult =
            Utils::queued_js_executor(long_running_func, delay.count());
    auto t2 = std::chrono::steady_clock::now();
    REQUIRE((t2 - t1) >= delay);
    REQUIRE(executionResult == 0);
}

TEST_CASE("Importing tfjs")
{
    auto executionResult = Utils::queued_js_executor(import_tfjs);
    REQUIRE(executionResult == 0);
}

TEST_CASE("Call to javascript while other threads are running")
{
    SUBCASE("There are remaining threads")
    {
        std::vector<std::thread> workers;
        for (auto i = 0; i < 6; ++i)
        {

            workers.emplace_back(std::thread(
                    []
                    {
                        std::cout
                                << "worker thread id: " << std::this_thread::get_id()
                                << std::endl;
                        auto i = 0;
                        while (i < 1000000000)
                        {
                            ++i;
                        }
                    }));
        }
        SUBCASE("Single call")
        {
            auto executionStatus = Utils::queued_js_executor(
                    log_data, "Hello World From remaining thread!");
            REQUIRE(
                    executionStatus == 0);
        }
        SUBCASE("Multiple calls")
        {
            for (auto i = 0; i < 100; ++i)
            {
                std::string log_string = "Hello, World from remaining thread! ";
                log_string += std::to_string(i);
                auto executionStatus = Utils::queued_js_executor(
                        log_data, log_string.c_str());
                REQUIRE(executionStatus ==
                        0);
            }
        }
        for (auto& t : workers)
        {
            t.join();
        }
    }

    SUBCASE("There are no remaining threads")
    // We are doing this with an assumed threadpool size of 8
    {
        std::vector<std::thread> workers;
        for (auto i = 0; i < 15; ++i)
        {

            workers.emplace_back(std::thread(
                    []
                    {
                        std::cout
                                << "worker thread id: " << std::this_thread::get_id()
                                << std::endl;
                        auto i = 0;
                        while (i < 1000000000)
                        {
                            ++i;
                        }
                    }));
        }
        SUBCASE("Single call")
        {
            auto executionStatus = Utils::queued_js_executor(
                    log_data, "Hello World from too many threads!");
            REQUIRE(
                    executionStatus == 0);
        }
        SUBCASE("Multiple calls")
        {
            for (auto i = 0; i < 100; ++i)
            {
                std::string log_string = "Hello, World from too many threads ";
                log_string += std::to_string(i);
                auto executionStatus = Utils::queued_js_executor(
                        log_data, log_string.c_str());
                REQUIRE(executionStatus ==
                        0);
            }
        }
        for (auto& t : workers)
        {
            t.join();
        }
    }
}

TEST_CASE("Calling js function from different C++ threads")
{
    std::vector<std::thread> workers;
    for (auto i = 0; i < 15; ++i)
    {

        workers.emplace_back(std::thread(
                [&]
                {
                    std::string log_string = "Hello, World from ";
                    std::stringstream ss;
                    ss << log_string;
                    ss << std::this_thread::get_id();
                    auto executionStatus = Utils::queued_js_executor(
                            log_data, ss.str().c_str());
                    REQUIRE(executionStatus ==
                            0);
                }));
    }

    for (auto& t : workers)
    {
        t.join();
    }
}

TEST_CASE("Calling long running js function from different C++ threads")
{
    std::vector<std::thread> workers;
    for (auto i = 0; i < 15; ++i)
    {

        workers.emplace_back(std::thread(
                [&]
                {
                    std::cout << "worker thread id: " << std::this_thread::get_id()
                              << std::endl;
                    auto delay           = std::chrono::milliseconds(1000);
                    auto t1              = std::chrono::steady_clock::now();
                    auto executionResult = Utils::queued_js_executor(
                            long_running_func, delay.count());
                    auto t2 = std::chrono::steady_clock::now();
                    REQUIRE((t2 - t1) >= delay);
                    REQUIRE(executionResult ==
                            0);
                }));
    }

    for (auto& t : workers)
    {
        t.join();
    }
}
