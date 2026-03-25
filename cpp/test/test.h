#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Register {
    Register(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct Failure {
    std::string message;
};

#define TEST(name) \
    static void test_##name(); \
    static test::Register reg_##name(#name, test_##name); \
    static void test_##name()

#define ASSERT_TRUE(expr) \
    do { if (!(expr)) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_TRUE(" #expr ") failed"}; } while(0)

#define ASSERT_FALSE(expr) \
    do { if ((expr)) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_FALSE(" #expr ") failed"}; } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_EQ(" #a ", " #b ") failed"}; } while(0)

#define ASSERT_NE(a, b) \
    do { if ((a) == (b)) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_NE(" #a ", " #b ") failed"}; } while(0)

#define ASSERT_GT(a, b) \
    do { if (!((a) > (b))) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_GT(" #a ", " #b ") failed"}; } while(0)

#define ASSERT_LT(a, b) \
    do { if (!((a) < (b))) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_LT(" #a ", " #b ") failed"}; } while(0)

#define ASSERT_NEAR(a, b, tol) \
    do { if (std::abs((a) - (b)) > (tol)) throw test::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": ASSERT_NEAR(" #a ", " #b ", " #tol ") failed: " + std::to_string(a) + " vs " + std::to_string(b)}; } while(0)

inline int run() {
    int passed = 0, failed = 0;
    for (auto& tc : registry()) {
        try {
            tc.fn();
            passed++;
        } catch (const Failure& f) {
            std::cerr << "FAIL " << tc.name << "\n  " << f.message << "\n";
            failed++;
        } catch (const std::exception& e) {
            std::cerr << "FAIL " << tc.name << "\n  exception: " << e.what() << "\n";
            failed++;
        }
    }
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}

} // namespace test
