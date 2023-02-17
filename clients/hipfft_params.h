// Copyright (C) 2021 - 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef HIPFFT_PARAMS_H
#define HIPFFT_PARAMS_H

#include "hipfft.h"
#include "hipfftXt.h"
#include "rocFFT/clients/fft_params.h"

inline fft_status fft_status_from_hipfftparams(const hipfftResult_t val)
{
    switch(val)
    {
    case HIPFFT_SUCCESS:
        return fft_status_success;
    case HIPFFT_INVALID_PLAN:
    case HIPFFT_ALLOC_FAILED:
        return fft_status_failure;
    case HIPFFT_INVALID_TYPE:
    case HIPFFT_INVALID_VALUE:
    case HIPFFT_INVALID_SIZE:
    case HIPFFT_INCOMPLETE_PARAMETER_LIST:
    case HIPFFT_INVALID_DEVICE:
    case HIPFFT_NOT_IMPLEMENTED:
    case HIPFFT_NOT_SUPPORTED:
        return fft_status_invalid_arg_value;
    case HIPFFT_INTERNAL_ERROR:
    case HIPFFT_EXEC_FAILED:
    case HIPFFT_SETUP_FAILED:
    case HIPFFT_UNALIGNED_DATA:
    case HIPFFT_PARSE_ERROR:
        return fft_status_failure;
    case HIPFFT_NO_WORKSPACE:
        return fft_status_invalid_work_buffer;
    default:
        return fft_status_failure;
    }
}

inline std::string hipfftResult_string(const hipfftResult_t val)
{
    switch(val)
    {
    case HIPFFT_SUCCESS:
        return "HIPFFT_SUCCESS (0)";
    case HIPFFT_INVALID_PLAN:
        return "HIPFFT_INVALID_PLAN (1)";
    case HIPFFT_ALLOC_FAILED:
        return "HIPFFT_ALLOC_FAILED (2)";
    case HIPFFT_INVALID_TYPE:
        return "HIPFFT_INVALID_TYPE (3)";
    case HIPFFT_INVALID_VALUE:
        return "HIPFFT_INVALID_VALUE (4)";
    case HIPFFT_INTERNAL_ERROR:
        return "HIPFFT_INTERNAL_ERROR (5)";
    case HIPFFT_EXEC_FAILED:
        return "HIPFFT_EXEC_FAILED (6)";
    case HIPFFT_SETUP_FAILED:
        return "HIPFFT_SETUP_FAILED (7)";
    case HIPFFT_INVALID_SIZE:
        return "HIPFFT_INVALID_SIZE (8)";
    case HIPFFT_UNALIGNED_DATA:
        return "HIPFFT_UNALIGNED_DATA (9)";
    case HIPFFT_INCOMPLETE_PARAMETER_LIST:
        return "HIPFFT_INCOMPLETE_PARAMETER_LIST (10)";
    case HIPFFT_INVALID_DEVICE:
        return "HIPFFT_INVALID_DEVICE (11)";
    case HIPFFT_PARSE_ERROR:
        return "HIPFFT_PARSE_ERROR (12)";
    case HIPFFT_NO_WORKSPACE:
        return "HIPFFT_NO_WORKSPACE (13)";
    case HIPFFT_NOT_IMPLEMENTED:
        return "HIPFFT_NOT_IMPLEMENTED (14)";
    case HIPFFT_NOT_SUPPORTED:
        return "HIPFFT_NOT_SUPPORTED (16)";
    default:
        return "invalid hipfftResult";
    }
}

class hipfft_params : public fft_params
{
public:
    // plan handles are pointers for rocFFT backend, and ints for cuFFT
#ifdef __HIP_PLATFORM_AMD__
    static constexpr hipfftHandle INVALID_PLAN_HANDLE = nullptr;
#else
    static constexpr hipfftHandle INVALID_PLAN_HANDLE = -1;
#endif

    hipfftHandle plan = INVALID_PLAN_HANDLE;

    hipfftType hipfft_transform_type;
    int        direction;

    std::vector<int> int_length;
    std::vector<int> int_inembed;
    std::vector<int> int_onembed;

    std::vector<long long int> ll_length;
    std::vector<long long int> ll_inembed;
    std::vector<long long int> ll_onembed;

    hipfft_params(){};

    hipfft_params(const fft_params& p)
        : fft_params(p){};

    ~hipfft_params()
    {
        free();
    };

    void free()
    {
        if(plan != INVALID_PLAN_HANDLE)
        {
            hipfftDestroy(plan);
            plan = INVALID_PLAN_HANDLE;
        }
    }

    size_t vram_footprint() override
    {
        size_t val = fft_params::vram_footprint();
        if(setup_structs() != fft_status_success)
        {
            throw std::runtime_error("Struct setup failed");
        }

        workbuffersize = 0;

        // Hack for estimating buffer requirements.
        workbuffersize = 3 * val;

        val += workbuffersize;
        return val;
    }

    fft_status setup_structs()
    {
        switch(transform_type)
        {
        case 0:
            hipfft_transform_type = (precision == fft_precision_single) ? HIPFFT_C2C : HIPFFT_Z2Z;
            direction             = HIPFFT_FORWARD;
            break;
        case 1:
            hipfft_transform_type = (precision == fft_precision_single) ? HIPFFT_C2C : HIPFFT_Z2Z;
            direction             = HIPFFT_BACKWARD;

            break;
        case 2:
            hipfft_transform_type = (precision == fft_precision_single) ? HIPFFT_R2C : HIPFFT_D2Z;
            direction             = HIPFFT_FORWARD;
            break;
        case 3:
            hipfft_transform_type = (precision == fft_precision_single) ? HIPFFT_C2R : HIPFFT_Z2D;
            direction             = HIPFFT_BACKWARD;
            break;
        default:
            throw std::runtime_error("Invalid transform type");
        }

        int_length.resize(dim());
        int_inembed.resize(dim());
        int_onembed.resize(dim());

        ll_length.resize(dim());
        ll_inembed.resize(dim());
        ll_onembed.resize(dim());
        switch(dim())
        {
        case 3:
            ll_inembed[2] = istride[1] / istride[2];
            ll_onembed[2] = ostride[1] / ostride[2];
            [[fallthrough]];
        case 2:
            ll_inembed[1] = istride[0] / istride[1];
            ll_onembed[1] = ostride[0] / ostride[1];
            [[fallthrough]];
        case 1:
            ll_inembed[0] = istride[dim() - 1];
            ll_onembed[0] = ostride[dim() - 1];
            break;
        default:
            throw std::runtime_error("Invalid dimension");
        }

        for(size_t i = 0; i < dim(); ++i)
        {
            ll_length[i]   = length[i];
            int_length[i]  = length[i];
            int_inembed[i] = ll_inembed[i];
            int_onembed[i] = ll_onembed[i];
        }

        hipfftResult ret = HIPFFT_SUCCESS;
        return fft_status_from_hipfftparams(ret);
    }

    fft_status create_plan() override
    {
        auto fft_ret = setup_structs();
        if(fft_ret != fft_status_success)
        {
            return fft_ret;
        }

        hipfftResult ret{HIPFFT_INTERNAL_ERROR};
        switch(get_create_type())
        {
        case PLAN_Nd:
        {
            ret = create_plan_Nd();
            break;
        }
        case PLAN_MANY:
        {
            ret = create_plan_many();
            break;
        }
        case CREATE_MAKE_PLAN_Nd:
        {
            ret = create_make_plan_Nd();
            break;
        }
        case CREATE_MAKE_PLAN_MANY:
        {
            ret = create_make_plan_many();
            break;
        }
        case CREATE_MAKE_PLAN_MANY64:
        {
            ret = create_make_plan_many64();
            break;
        }
        default:
        {
            throw std::runtime_error("no valid plan creation type");
        }
        }
        return fft_status_from_hipfftparams(ret);
    }

    fft_status set_callbacks(void* load_cb_host,
                             void* load_cb_data,
                             void* store_cb_host,
                             void* store_cb_data) override
    {
        if(run_callbacks)
        {
            hipfftResult ret{HIPFFT_EXEC_FAILED};
            switch(hipfft_transform_type)
            {
            case HIPFFT_R2C:
                ret = hipfftXtSetCallback(plan, &load_cb_host, HIPFFT_CB_LD_REAL, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(
                    plan, &store_cb_host, HIPFFT_CB_ST_COMPLEX, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            case HIPFFT_D2Z:
                ret = hipfftXtSetCallback(
                    plan, &load_cb_host, HIPFFT_CB_LD_REAL_DOUBLE, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(
                    plan, &store_cb_host, HIPFFT_CB_ST_COMPLEX_DOUBLE, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            case HIPFFT_C2R:
                ret = hipfftXtSetCallback(plan, &load_cb_host, HIPFFT_CB_LD_COMPLEX, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(plan, &store_cb_host, HIPFFT_CB_ST_REAL, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            case HIPFFT_Z2D:
                ret = hipfftXtSetCallback(
                    plan, &load_cb_host, HIPFFT_CB_LD_COMPLEX_DOUBLE, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(
                    plan, &store_cb_host, HIPFFT_CB_ST_REAL_DOUBLE, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            case HIPFFT_C2C:
                ret = hipfftXtSetCallback(plan, &load_cb_host, HIPFFT_CB_LD_COMPLEX, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(
                    plan, &store_cb_host, HIPFFT_CB_ST_COMPLEX, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            case HIPFFT_Z2Z:
                ret = hipfftXtSetCallback(
                    plan, &load_cb_host, HIPFFT_CB_LD_COMPLEX_DOUBLE, &load_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);

                ret = hipfftXtSetCallback(
                    plan, &store_cb_host, HIPFFT_CB_ST_COMPLEX_DOUBLE, &store_cb_data);
                if(ret != HIPFFT_SUCCESS)
                    return fft_status_from_hipfftparams(ret);
                break;
            default:
                throw std::runtime_error("Invalid execution type");
            }
        }
        return fft_status_success;
    }

    virtual fft_status execute(void** in, void** out) override
    {
        return execute(in[0], out[0]);
    };

    fft_status execute(void* ibuffer, void* obuffer)
    {
        hipfftResult ret{HIPFFT_EXEC_FAILED};
        try
        {
            switch(hipfft_transform_type)
            {
            case HIPFFT_R2C:
                ret = hipfftExecR2C(
                    plan,
                    (hipfftReal*)ibuffer,
                    (hipfftComplex*)(placement == fft_placement_inplace ? ibuffer : obuffer));
                break;
            case HIPFFT_D2Z:
                ret = hipfftExecD2Z(
                    plan,
                    (hipfftDoubleReal*)ibuffer,
                    (hipfftDoubleComplex*)(placement == fft_placement_inplace ? ibuffer : obuffer));
                break;
            case HIPFFT_C2R:
                ret = hipfftExecC2R(
                    plan,
                    (hipfftComplex*)ibuffer,
                    (hipfftReal*)(placement == fft_placement_inplace ? ibuffer : obuffer));
                break;
            case HIPFFT_Z2D:
                ret = hipfftExecZ2D(
                    plan,
                    (hipfftDoubleComplex*)ibuffer,
                    (hipfftDoubleReal*)(placement == fft_placement_inplace ? ibuffer : obuffer));
                break;
            case HIPFFT_C2C:
                ret = hipfftExecC2C(
                    plan,
                    (hipfftComplex*)ibuffer,
                    (hipfftComplex*)(placement == fft_placement_inplace ? ibuffer : obuffer),
                    direction);
                break;
            case HIPFFT_Z2Z:
                ret = hipfftExecZ2Z(
                    plan,
                    (hipfftDoubleComplex*)ibuffer,
                    (hipfftDoubleComplex*)(placement == fft_placement_inplace ? ibuffer : obuffer),
                    direction);
                break;
            default:
                throw std::runtime_error("Invalid execution type");
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        catch(...)
        {
            std::cerr << "unkown exception in execute(void* ibuffer, void* obuffer)" << std::endl;
        }
        return fft_status_from_hipfftparams(ret);
    }

    bool is_contiguous() const
    {
        // compute contiguous stride, dist and check that the actual
        // strides/dists match
        std::vector<size_t> contiguous_istride
            = compute_stride(ilength(),
                             {},
                             placement == fft_placement_inplace
                                 && transform_type == fft_transform_type_real_forward);
        std::vector<size_t> contiguous_ostride
            = compute_stride(olength(),
                             {},
                             placement == fft_placement_inplace
                                 && transform_type == fft_transform_type_real_inverse);
        if(istride != contiguous_istride || ostride != contiguous_ostride)
            return false;
        return compute_idist() == idist && compute_odist() == odist;
    }

private:
    // hipFFT provides multiple ways to create FFT plans:
    // - hipfftPlan1d/2d/3d (combined allocate + init for specific dim)
    // - hipfftPlanMany (combined allocate + init with dim as param)
    // - hipfftCreate + hipfftMakePlan1d/2d/3d (separate alloc + init for specific dim)
    // - hipfftCreate + hipfftMakePlanMany (separate alloc + init with dim as param)
    // - hipfftCreate + hipfftMakePlanMany64 (separate alloc + init with dim as param, 64-bit)
    //
    // Rotate through the choices for better test coverage.
    enum PlanCreateAPI
    {
        PLAN_Nd,
        PLAN_MANY,
        CREATE_MAKE_PLAN_Nd,
        CREATE_MAKE_PLAN_MANY,
        CREATE_MAKE_PLAN_MANY64,
    };

    // Not all plan options work with all creation types.  Return a
    // suitable plan creation type for the current FFT parameters.
    int get_create_type()
    {
        bool contiguous = is_contiguous();
        bool batched    = nbatch > 1;

        std::vector<PlanCreateAPI> allowed_apis;

        // separate alloc + init "Many" APIs are always allowed
        allowed_apis.push_back(CREATE_MAKE_PLAN_MANY);
        allowed_apis.push_back(CREATE_MAKE_PLAN_MANY64);

        // combined PlanMany API can't do scaling
        if(scale_factor == 1.0)
            allowed_apis.push_back(PLAN_MANY);

        // non-many APIs are only allowed if FFT is contiguous, and
        // only the 1D API allows for batched FFTs.
        if(contiguous && (!batched || dim() == 1))
        {
            // combined Nd API can't do scaling
            if(scale_factor == 1.0)
                allowed_apis.push_back(PLAN_Nd);
            allowed_apis.push_back(CREATE_MAKE_PLAN_Nd);
        }

        // hash the token to decide how to create this FFT.  we want
        // test cases to rotate between different create APIs, but we
        // also need the choice of API to be stable across reruns of
        // the same test cases.
        return allowed_apis[std::hash<std::string>()(token()) % allowed_apis.size()];
    }

    // call hipfftPlan* functions
    hipfftResult_t create_plan_Nd()
    {
        auto ret = HIPFFT_INVALID_PLAN;
        switch(dim())
        {
        case 1:
            ret = hipfftPlan1d(&plan, int_length[0], hipfft_transform_type, nbatch);
            break;
        case 2:
            ret = hipfftPlan2d(&plan, int_length[0], int_length[1], hipfft_transform_type);
            break;
        case 3:
            ret = hipfftPlan3d(
                &plan, int_length[0], int_length[1], int_length[2], hipfft_transform_type);
            break;
        default:
            throw std::runtime_error("invalid dim");
        }
        return ret;
    }
    hipfftResult_t create_plan_many()
    {
        auto ret = hipfftPlanMany(&plan,
                                  dim(),
                                  int_length.data(),
                                  int_inembed.data(),
                                  istride.back(),
                                  idist,
                                  int_onembed.data(),
                                  ostride.back(),
                                  odist,
                                  hipfft_transform_type,
                                  nbatch);
        return ret;
    }

    // call hipfftCreate + hipfftMake* functions
    hipfftResult_t create_with_scale_factor()
    {
        auto ret = hipfftCreate(&plan);
        if(ret != HIPFFT_SUCCESS)
            return ret;
        if(scale_factor != 1.0)
        {
            ret = hipfftExtPlanScaleFactor(plan, scale_factor);
            if(ret != HIPFFT_SUCCESS)
                return ret;
        }
        return ret;
    }
    hipfftResult_t create_make_plan_Nd()
    {
        auto ret = create_with_scale_factor();
        if(ret != HIPFFT_SUCCESS)
            return ret;

        switch(dim())
        {
        case 1:
            return hipfftMakePlan1d(
                plan, int_length[0], hipfft_transform_type, nbatch, &workbuffersize);
        case 2:
            return hipfftMakePlan2d(
                plan, int_length[0], int_length[1], hipfft_transform_type, &workbuffersize);
        case 3:
            return hipfftMakePlan3d(plan,
                                    int_length[0],
                                    int_length[1],
                                    int_length[2],
                                    hipfft_transform_type,
                                    &workbuffersize);
        default:
            throw std::runtime_error("invalid dim");
        }
    }
    hipfftResult_t create_make_plan_many()
    {
        auto ret = create_with_scale_factor();
        if(ret != HIPFFT_SUCCESS)
            return ret;
        return hipfftMakePlanMany(plan,
                                  dim(),
                                  int_length.data(),
                                  int_inembed.data(),
                                  istride.back(),
                                  idist,
                                  int_onembed.data(),
                                  ostride.back(),
                                  odist,
                                  hipfft_transform_type,
                                  nbatch,
                                  &workbuffersize);
    }
    hipfftResult_t create_make_plan_many64()
    {
        auto ret = create_with_scale_factor();
        if(ret != HIPFFT_SUCCESS)
            return ret;
        return hipfftMakePlanMany64(plan,
                                    dim(),
                                    ll_length.data(),
                                    ll_inembed.data(),
                                    istride.back(),
                                    idist,
                                    ll_onembed.data(),
                                    ostride.back(),
                                    odist,
                                    hipfft_transform_type,
                                    nbatch,
                                    &workbuffersize);
    }
};

#endif
