#ifndef TESTS_H
#define TESTS_H

#define CURRENT_CHECK_POINT 3

// macros for testing check point 1
// continue test with error: 0 1 1
// continue test without error: 0 1 0
// rtc test: 1 x x
// separate test mode: 0 0 x
#define RTC_ENABLE_PRINT 0
#define CONTINUE_ERROR_TESTS 0
#define TEST_WITH_ERROR 0

#define MAX_TERMINAL_BUFFER 129
#define RTC_TEST_LEN        10
#define SCROLLING_TEST_MAX  200

// test launcher
void launch_tests();

void continue_testing();

#endif /* TESTS_H */
