/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xla/service/gpu/kernels/cutlass_gemm_custom_kernel.h"

#include <cstdint>
#include <utility>

#include "absl/status/status.h"
#include "xla/service/gpu/kernels/custom_kernel.h"
#include "xla/service/gpu/kernels/cutlass_gemm_kernels.cu.h"
#include "xla/service/gpu/kernels/cutlass_gemm_universal.cu.h"
#include "xla/statusor.h"
#include "xla/stream_executor/kernel_spec.h"
#include "xla/xla_data.pb.h"

namespace xla::gpu::kernel {

template <typename Gemm>
static StatusOr<CustomKernel> LoadCutlassGemmUniversal(int32_t m, int32_t n,
                                                       int32_t k) {
  using Kernel = typename Gemm::GemmKernel;

  cutlass::gemm::GemmCoord problem_size = {m, n, k};

  se::MultiKernelLoaderSpec spec(
      /*arity=*/1, gemm_universal::ArgsPacking<Gemm>(problem_size));
  spec.AddInProcessSymbol(internal::GetCutlassGemmKernel<Gemm>(),
                          "cutlass_gemm");

  return CustomKernel("cutlass_gemm", std::move(spec),
                      gemm_universal::BlockDim<Gemm>(problem_size),
                      gemm_universal::ThreadDim<Gemm>(),
                      sizeof(typename Kernel::SharedStorage));
}

StatusOr<CustomKernel> GetCutlassGemmKernel(PrimitiveType dtype, int32_t m,
                                            int32_t n, int32_t k) {
  if (dtype != PrimitiveType::F32)
    return absl::InvalidArgumentError(
        "Currently cutlass gemm kernel supports only F32 data type");

  return LoadCutlassGemmUniversal<CutlassGemmKernels::F32xF32toF32>(m, n, k);
}

}  // namespace xla::gpu::kernel
