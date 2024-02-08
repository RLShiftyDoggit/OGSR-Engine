#include "stdafx.h"
#include "dx103DFluidBlenders.h"
#include "dx103DFluidManager.h"
#include "dx103DFluidRenderer.h"

// Volume texture width
static class cl_textureWidth final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float tW = (float)FluidManager.GetTextureWidth();
        RCache.set_c(C, tW);
    }
} binder_textureWidth;

// Volume texture height
static class cl_textureHeight final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float tH = (float)FluidManager.GetTextureHeight();
        RCache.set_c(C, tH);
    }
} binder_textureHeight;

// Volume texture depth
static class cl_textureDepth final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, tD);
    }
} binder_textureDepth;

static class cl_gridDim final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float tW = (float)FluidManager.GetTextureWidth();
        float tH = (float)FluidManager.GetTextureHeight();
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, tW, tH, tD, 0.0f);
    }
} binder_gridDim;

static class cl_recGridDim final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        float tW = (float)FluidManager.GetTextureWidth();
        float tH = (float)FluidManager.GetTextureHeight();
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, 1.0f / tW, 1.0f / tH, 1.0f / tD, 0.0f);
    }
} binder_recGridDim;

static class cl_maxDim final : public R_constant_setup
{
    void setup(R_constant* C) override
    {
        int tW = FluidManager.GetTextureWidth();
        int tH = FluidManager.GetTextureHeight();
        int tD = FluidManager.GetTextureDepth();
        float tMax = (float)_max(tW, _max(tH, tD));
        RCache.set_c(C, (float)tMax);
    }
} binder_maxDim;


void BindConstants(CBlender_Compile& C)
{
    C.r_Constant("textureWidth", &binder_textureWidth);
    C.r_Constant("textureHeight", &binder_textureHeight);
    C.r_Constant("textureDepth", &binder_textureDepth);

    C.r_Constant("gridDim", &binder_gridDim);
    C.r_Constant("recGridDim", &binder_recGridDim);
    C.r_Constant("maxGridDim", &binder_maxDim);
}

void SetupSamplers(CBlender_Compile& C)
{
    int smp = C.r_dx10Sampler("samPointClamp");
    if (smp != u32(-1))
    {
        C.i_dx10Address(smp, D3DTADDRESS_CLAMP);
        C.i_dx10Filter(smp, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT);
    }

    smp = C.r_dx10Sampler("samLinear");
    if (smp != u32(-1))
    {
        C.i_dx10Address(smp, D3DTADDRESS_CLAMP);
        C.i_dx10Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    smp = C.r_dx10Sampler("samLinearClamp");
    if (smp != u32(-1))
    {
        C.i_dx10Address(smp, D3DTADDRESS_CLAMP);
        C.i_dx10Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    smp = C.r_dx10Sampler("samRepeat");
    if (smp != u32(-1))
    {
        C.i_dx10Address(smp, D3DTADDRESS_WRAP);
        C.i_dx10Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }
}

void SetupTextures(CBlender_Compile& C)
{
    for (int i = 0; i < dx103DFluidManager::NUM_RENDER_TARGETS; ++i)
        C.r_dx10Texture(dx103DFluidConsts::m_pShaderTextureNames[i], dx103DFluidConsts::m_pEngineTextureNames[i]);

    //	Renderer
    C.r_dx10Texture("sceneDepthTex", r2_RT_P);
    C.r_dx10Texture("colorTex", dx103DFluidConsts::m_pEngineTextureNames[dx103DFluidManager::RENDER_TARGET_COLOR_IN]);
    C.r_dx10Texture("jitterTex", "$user$NVjitterTex");

    C.r_dx10Texture("HHGGTex", "$user$NVHHGGTex");

    C.r_dx10Texture("fireTransferFunction", "internal\\internal_fireTransferFunction");

    for (int i = 0; i < dx103DFluidRenderer::RRT_NumRT; ++i)
        C.r_dx10Texture(dx103DFluidConsts::m_pResourceRTNames[i], dx103DFluidConsts::m_pRTNames[i]);
}

void CBlender_fluid_advect::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // Advect
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect", false, FALSE, FALSE, FALSE);
        break;
    case 1: // AdvectBFECC
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_bfecc", false, FALSE, FALSE, FALSE);
        break;
    case 2: // AdvectTemp
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_temp", false, FALSE, FALSE, FALSE);
        break;
    case 3: // AdvectBFECCTemp
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_bfecc_temp", false, FALSE, FALSE, FALSE);
        break;
    case 4: // AdvectVel
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound befor r_End()
    C.r_End();
}

void CBlender_fluid_advect_velocity::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // AdvectVel
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel", false, FALSE, FALSE, FALSE);
        break;
    case 1: // AdvectVelGravity
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel_g", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound befor r_End()
    C.r_End();
}

void CBlender_fluid_simulate::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // Vorticity
        C.r_Pass("fluid_grid", "fluid_array", "fluid_vorticity", false, FALSE, FALSE, FALSE);
        break;
    case 1: // Confinement
        //	Use additive blending
        C.r_Pass("fluid_grid", "fluid_array", "fluid_confinement", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        break;
    case 2: // Divergence
        C.r_Pass("fluid_grid", "fluid_array", "fluid_divergence", false, FALSE, FALSE, FALSE);
        break;
    case 3: // Jacobi
        C.r_Pass("fluid_grid", "fluid_array", "fluid_jacobi", false, FALSE, FALSE, FALSE);
        break;
    case 4: // Project
        C.r_Pass("fluid_grid", "fluid_array", "fluid_project", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_obst::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // ObstStaticBox
        //	OOBB
        C.r_Pass("fluid_grid_oobb", "fluid_array_oobb", "fluid_obst_static_oobb", false, FALSE, FALSE, FALSE);
        break;
    case 1: // ObstDynBox
        //	OOBB
        C.r_Pass("fluid_grid_dyn_oobb", "fluid_array_dyn_oobb", "fluid_obst_dynamic_oobb", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_emitter::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // ET_SimpleGausian
        C.r_Pass("fluid_grid", "fluid_array", "fluid_gaussian", false, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        C.RS.SetRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);
        C.RS.SetRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_obstdraw::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // DrawTexture
        C.r_Pass("fluid_grid", "null", "fluid_draw_texture", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_raydata::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // CompRayData_Back
        C.r_Pass("fluid_raydata_back", "null", "fluid_raydata_back", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CW); //	Front
        break;
    case 1: // CompRayData_Front
        C.r_Pass("fluid_raydata_front", "null", "fluid_raydata_front", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        // RS.SetRS(D3DRS_SRCBLENDALPHA,		bABlend?abSRC:D3DBLEND_ONE	);
        //	We need different blend arguments for color and alpha
        //	One Zero for color
        //	One One for alpha
        //	so patch dest color.
        //	Note: You can't set up dest blend to zero in r_pass
        //	since r_pass would disable blend if src=one and blend - zero.
        C.RS.SetRS(D3DRS_DESTBLEND, D3DBLEND_ZERO);

        C.RS.SetRS(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT); // DST - SRC
        C.RS.SetRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_REVSUBTRACT); // DST - SRC

        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 2: // QuadDownSampleRayDataTexture
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raydatacopy_quad", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    }

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_raycast::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // QuadEdgeDetect
        C.r_Pass("fluid_edge_detect", "null", "fluid_edge_detect", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_NONE); //	Back
        break;
    case 1: // QuadRaycastFog
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycast_quad", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 2: // QuadRaycastCopyFog
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycastcopy_quad", false, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        C.r_ColorWriteEnable(true, true, true, false);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 3: // QuadRaycastFire
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycast_quad_fire", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 4: // QuadRaycastCopyFire
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycastcopy_quad_fire", false, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        C.r_ColorWriteEnable(true, true, true, false);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    }

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}
