//benchmark copied from  https://cedardb.com/blog/exceptions_vs_errors/
#include <benchmark/benchmark.h>

#include "match.hpp"

#include <expected>
#include <string>

using namespace cppmatch;

struct invalid_value { std::string reason; };

unsigned do_fib_throws(unsigned n, unsigned max_depth) {
   if (!max_depth) throw invalid_value(std::to_string(n) + " exceeds max_depth");
   if (n <= 2) return 1;
   return do_fib_throws(n - 2, max_depth - 1) + do_fib_throws(n - 1, max_depth - 1);
}

std::expected<unsigned, invalid_value> do_fib_expected(unsigned n, unsigned max_depth) {
   if (!max_depth) return std::unexpected<invalid_value>(std::to_string(n) + " exceeds max_depth");
   if (n <= 2) return 1;
   auto n2 = do_fib_expected(n - 2, max_depth - 1);
   if (!n2) return std::unexpected(n2.error());
   auto n1 = do_fib_expected(n - 1, max_depth - 1);
   if (!n1) return std::unexpected(n1.error());
   return *n1 + *n2;
}

Result<unsigned, invalid_value> do_fib_cppmatch(unsigned n, unsigned max_depth) {
   if (!max_depth) return invalid_value{std::to_string(n) + " exceeds max_depth"};
   if (n <= 2) return 1U;
   return expect(do_fib_cppmatch(n - 2, max_depth - 1)) + expect(do_fib_cppmatch(n - 1, max_depth - 1));
}

static void fib_cppmatch(benchmark::State& state) {
  for (auto _ : state) {
    auto res = match(do_fib_cppmatch(15, 20),
        [](invalid_value) { return 0U; },
        [](unsigned n) { return n; }
    );
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(fib_cppmatch);


static void fib_throws(benchmark::State& state) {
  for (auto _ : state) {
    auto res = do_fib_throws(15, 20);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(fib_throws);

static void fib_expected(benchmark::State& state) {
  for (auto _ : state) {
    auto res = do_fib_expected(15, 20);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(fib_expected);


BENCHMARK_MAIN();