#include <system_error>

#include "ws_client/errors.hpp"

namespace ws_client::test
{
const std::error_category& error_category_from_other_translation_unit()
{
    return error_category;
}

std::error_code error_code_from_other_translation_unit(WSErrorCode code)
{
    return make_error_code(code);
}
} // namespace ws_client::test
