// constraint_check.hpp - Shared compile-time failure path for ada:: constraint checks.

#ifndef ADA_DETAIL_CONSTRAINT_CHECK_HPP
#define ADA_DETAIL_CONSTRAINT_CHECK_HPP

namespace ada::detail {

// Not constexpr: reaching it during constant evaluation makes the expression ill-formed, even with NDEBUG.
// Named after Ada's Constraint_Error, which the original code would raise at runtime.
inline void constraint_error_in_constant_expression() noexcept {}

} // namespace ada::detail

#endif // ADA_DETAIL_CONSTRAINT_CHECK_HPP
