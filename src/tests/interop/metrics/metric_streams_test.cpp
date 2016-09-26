/** Unit tests for the metric streams
 *
 *
 *  @file
 *  @date 9/17/2016
 *  @version 1.0
 *  @copyright GNU Public License.
 */

#ifdef _MSC_VER
#pragma warning(disable:4127) // MSVC warns about using constants in conditional statements, for template constants
#endif

#include <gtest/gtest.h>
#include "src/tests/interop/metrics/inc/metric_format_fixtures.h"

using namespace illumina::interop;
using namespace illumina::interop::unittest;


/** Fixture for expected vs actual binary data */
template<typename TestSetup>
struct metric_stream_test : public ::testing::Test, public TestSetup
{
    /** Type of metric set */
    typedef typename TestSetup::metric_set_t metric_set_t;
    /** Constructor */
    metric_stream_test()
    {
        TestSetup::create_binary_data(expected);
        metric_set_t metrics;
        TestSetup::create_metric_set(metrics);
        std::ostringstream fout;
        io::write_metrics(fout, metrics);
        actual = fout.str();
    }
    /** Expected binary data */
    std::string expected;
    /** Actual binary data */
    std::string actual;
};
TYPED_TEST_CASE_P(metric_stream_test);

/**
 * Confirm binary write matches expected binary data
 */
TYPED_TEST_P(metric_stream_test, test_write_read_binary_data)
{
    if (TypeParam::disable_binary_data_size) return;
    ASSERT_EQ(TestFixture::expected.size(), TestFixture::actual.size());
    if (TypeParam::disable_binary_data) return;
    for (::uint32_t i = 0;i<TestFixture::expected.size(); ++i)
    {
        ASSERT_EQ(int(TestFixture::expected[i]), int(TestFixture::actual[i])) << " i= " << i;
    }
}

/** Confirm bad_format_exception is thrown when version is unsupported
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_bad_format_exception)
{
    std::string tmp = std::string(TestFixture::expected);
    tmp[0] = 34;
    typename TypeParam::metric_set_t metrics;
    EXPECT_THROW(io::read_interop_from_string(tmp, metrics), io::bad_format_exception);
}


/** Confirm incomplete_file_exception is thrown for a small partial record
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_incomplete_file_exception)
{
    ::uint32_t incomplete = 0;
    for (::uint32_t i = 2; i < 25; i++)
    {
        typename TypeParam::metric_set_t metrics;
        try
        {
            io::read_interop_from_string(TestFixture::expected.substr(0, i), metrics);
        }
        catch (const io::incomplete_file_exception &)
        { incomplete++; }
        catch (const std::exception &ex)
        {
            std::cerr << i << " " << ex.what() << std::endl;
            throw;
        }
    }
    EXPECT_TRUE(incomplete > 10) << "incomplete: " << incomplete;
}
/** Confirm incomplete_file_exception is thrown for a mostly complete file
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_incomplete_file_exception_last_metric)
{
    typename TypeParam::metric_set_t metrics;
    EXPECT_THROW(io::read_interop_from_string(
            TestFixture::expected.substr(0, TestFixture::expected.length() - 4), metrics),
                 io::incomplete_file_exception);
}
/** Confirm bad_format_exception is thrown when record size is incorrect
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_incorrect_record_size)
{
    if (TypeParam::disable_check_record_size) return;
    std::string tmp = std::string(TestFixture::expected);
    tmp[1] = 0;
    tmp[2] = 0;
    typename TypeParam::metric_set_t metrics;
    EXPECT_THROW(io::read_interop_from_string(tmp, metrics), io::bad_format_exception);
}
/** Confirm file_not_found_exception is thrown when a file is not found
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_file_not_found)
{
    typename TypeParam::metric_set_t metrics;
    EXPECT_THROW(io::read_interop("/NO/FILE/EXISTS", metrics), io::file_not_found_exception);
}
/** Confirm reading from good data does not throw an exception
 */
TYPED_TEST_P(metric_stream_test, test_hardcoded_read)
{
    std::string tmp = std::string(TestFixture::expected);
    typename TypeParam::metric_set_t metrics;
    EXPECT_NO_THROW(io::read_interop_from_string(tmp, metrics));
}

REGISTER_TYPED_TEST_CASE_P(metric_stream_test, test_write_read_binary_data,
                           test_hardcoded_bad_format_exception,
                           test_hardcoded_incomplete_file_exception,
                           test_hardcoded_incomplete_file_exception_last_metric,
                           test_hardcoded_incorrect_record_size,
                           test_hardcoded_file_not_found,
                           test_hardcoded_read
);


INSTANTIATE_TYPED_TEST_CASE_P(Public, metric_stream_test, PublicFormats);

