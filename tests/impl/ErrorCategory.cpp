#include <gtest/gtest.h>

#include <system_error>

#include "ws_client/errors.hpp"

namespace ws_client::test
{
const std::error_category& error_category_from_other_translation_unit();
std::error_code error_code_from_other_translation_unit(WSErrorCode code);
} // namespace ws_client::test

TEST(ErrorCategory, identity_is_shared_across_translation_units)
{
    EXPECT_EQ(
        &ws_client::error_category, &ws_client::test::error_category_from_other_translation_unit()
    );
    EXPECT_EQ(
        ws_client::make_error_code(ws_client::WSErrorCode::protocol_error),
        ws_client::test::error_code_from_other_translation_unit(
            ws_client::WSErrorCode::protocol_error
        )
    );
}
