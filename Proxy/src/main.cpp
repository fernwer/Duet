// --- START OF FILE main.cpp ---
#include "scheduler.hpp"
#include <iostream>
#include <vector>
#include <thread>

int main() {
    std::cout << "=== Lumos Proxy (Integration Test Mode) Started ===" << std::endl;

    std::string conn_str = "dbname=tpch user=postgres password=Sjtu123 host=localhost port=5432";

    BatchScheduler scheduler(100, 10, true, conn_str);

    std::vector<std::string> test_queries = {
        "SELECT * FROM customer WHERE c_custkey = 101",
        "SELECT * FROM customer WHERE c_custkey = 102",

        "SELECT count(*) FROM orders WHERE o_orderdate > '1995-01-01' AND o_totalprice > 100.0",
        "SELECT count(*) FROM orders WHERE o_orderdate > '1996-01-01' AND o_totalprice > 200.0"};

    std::cout << ">>> Sending Probing Queries to Kernel via mqo_debug..." << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < test_queries.size(); ++i) {
        threads.emplace_back([&scheduler, i, &test_queries]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 5));
            scheduler.Submit(i, test_queries[i]);
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}