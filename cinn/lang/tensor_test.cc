#include "cinn/lang/tensor.h"

#include <gtest/gtest.h>

#include "cinn/ir/function.h"
#include "cinn/ir/ir_operators.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/lang/compute.h"
#include "cinn/lang/lower.h"
#include "cinn/lang/placeholder.h"

namespace cinn {
namespace ir {
using utils::GetStreamCnt;
using utils::Trim;

TEST(Tensor, inlined) {
  lang::Placeholder<float> A("A", {100, 20});
  lang::Placeholder<float> B("B", {100, 20});

  lang::Buffer D_buf;
  // C is inlined
  Tensor C = lang::Compute(
      {100, 20}, [=](Var i, Var j) { return A(i, j) + B(i, j); }, "C");
  Tensor D = lang::Compute(
      {100, 20}, [=](Var i, Var j) -> Expr { return C(i, j) * 2.f + 1.f; }, "D");
  D->Bind(D_buf);

  auto funcs = lang::Lower("func_C", {A, B, D});
  ASSERT_EQ(funcs.size(), 1UL);
  LOG(INFO) << "output: \n" << funcs.front();
  auto out = GetStreamCnt(funcs.front());
  EXPECT_EQ(Trim(out), Trim(R"ROC(
function func_C (A, B, D)
{
  poly_for (0, (c1 <= 99), 1)
  {
    poly_for (0, (c3 <= 19), 1)
    {
      D[((c1 * 20) + c3)] = (((A(c1, c3) + B(c1, c3)) * 2) + 1)
    }
  }
}
)ROC"));
}

}  // namespace ir
}  // namespace cinn