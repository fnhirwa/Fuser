// clang-format off
/*
 * SPDX-FileCopyrightText: Copyright (c) 2023-present NVIDIA CORPORATION & AFFILIATES.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
// clang-format on

#include <ir/utils.h>
#include <scheduler/debug_utils.h>
#include <scheduler/expr_eval_sched.h>
#include <scheduler/registry_utils.h>

namespace nvfuser {

// Check if the fusion has a single MatmulOp/LinearOp node
bool ExprEvalScheduler::canScheduleCompileTime(Fusion* fusion) {
  if (scheduler_utils::isResharding(fusion)) {
    scheduler_debug_utils::canScheduleRejectReason(
        heuristicType(), "Fusion is resharding.");
    return false;
  }

  auto exprs = fusion->exprs();
  if (exprs.size() != 1) {
    scheduler_debug_utils::canScheduleRejectReason(
        heuristicType(), "Fusion must contain only a single expression.");
    return false;
  }

  if (exprs.front()->isOneOf<SdpaFwdOp, SdpaBwdOp>()) {
    return true;
  }

  if (exprs.front()->isOneOf<LinearOp, MatmulOp>()) {
    if (isOptionDisabled(DisableOption::MatmulExprEval)) {
      scheduler_debug_utils::canScheduleRejectReason(
          heuristicType(),
          "Matmul ATen evaluation was disabled by NVFUSER_DISABLE=matmul_expr_eval");
      return false;
    }
    return true;
  }

  scheduler_debug_utils::canScheduleRejectReason(
      heuristicType(),
      "Fusion must contain only a single expression of type MatmulOp/LinearOp/SdpaFwdOp/SdpaBwdOp");
  return false;
}

void ExprEvalScheduler::schedule(Fusion* fusion) {
  std::for_each(
      fusion->outputs().begin(), fusion->outputs().end(), [&](Val* out) {
        fusion->aliasOutputToInput(
            out, /*input=*/nullptr, AllocationType::Evaluate);
      });
}

} // namespace nvfuser
