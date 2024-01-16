#include "stdafx.h"


#include "../../xr_3da/igame_persistent.h"
#include "../../xr_3da/igame_level.h"
#include "../../xr_3da/environment.h"
#include "../../xr_3da/fmesh.h"

#include "ftreevisual.h"

FTreeVisual::FTreeVisual(void) {}

FTreeVisual::~FTreeVisual(void) {}

void FTreeVisual::Release() { dxRender_Visual::Release(); }

void FTreeVisual::Load(const char* N, IReader* data, u32 dwFlags)
{
    dxRender_Visual::Load(N, data, dwFlags);

    D3DVERTEXELEMENT9* vFormat = NULL;

    // read vertices
    R_ASSERT(data->find_chunk(OGF_GCONTAINER));
    {
        // verts
        u32 ID = data->r_u32();
        vBase = data->r_u32();
        vCount = data->r_u32();
        vFormat = RImplementation.getVB_Format(ID);

        VERIFY(NULL == p_rm_Vertices);

        p_rm_Vertices = RImplementation.getVB(ID);
        p_rm_Vertices->AddRef();

        // indices
        dwPrimitives = 0;
        ID = data->r_u32();
        iBase = data->r_u32();
        iCount = data->r_u32();
        dwPrimitives = iCount / 3;

        VERIFY(NULL == p_rm_Indices);
        p_rm_Indices = RImplementation.getIB(ID);
        p_rm_Indices->AddRef();
    }

    // load tree-def
    R_ASSERT(data->find_chunk(OGF_TREEDEF2));
    {
        data->r(&xform, sizeof(xform));
        data->r(&c_scale, sizeof(c_scale));
        c_scale.rgb.mul(.5f);
        c_scale.hemi *= .5f;
        c_scale.sun *= .5f;
        data->r(&c_bias, sizeof(c_bias));
        c_bias.rgb.mul(.5f);
        c_bias.hemi *= .5f;
        c_bias.sun *= .5f;
        // Msg				("hemi[%f / %f], sun[%f / %f]",c_scale.hemi,c_bias.hemi,c_scale.sun,c_bias.sun);
    }

    // Geom
    rm_geom.create(vFormat, p_rm_Vertices, p_rm_Indices);
}

struct FTreeVisual_setup
{
    u32 dwFrame;
    float scale{};
    Fvector4 wave;
    Fvector4 wind;

    FTreeVisual_setup() { dwFrame = 0; }

    void calculate()
    {
        dwFrame = Device.dwFrame;
        float tm_rot = PI_MUL_2 * Device.fTimeGlobal / ps_r__Tree_w_rot;

        // Calc wind-vector3, scale
        CEnvDescriptor& env = *g_pGamePersistent->Environment().CurrentEnv;
        wind.set(_sin(tm_rot), 0, _cos(tm_rot), 0);
        wind.normalize();
        wind.mul(env.m_fTreeAmplitudeIntensity); // dir1*amplitude
        scale = 1.f / FTreeVisual_quant;

        // setup constants
        wave.set(ps_r__Tree_Wave.x, ps_r__Tree_Wave.y, ps_r__Tree_Wave.z, Device.fTimeGlobal * ps_r__Tree_w_speed); // wave
        wave.div(PI_MUL_2);
    }
};

void FTreeVisual::Render(float LOD)
{
    static FTreeVisual_setup tvs;
    if (tvs.dwFrame != Device.dwFrame)
        tvs.calculate();
        // setup constants

    Fmatrix xform_v;
    xform_v.mul_43(RCache.get_xform_view(), xform);
    RCache.tree.set_m_xform_v(xform_v); // matrix

    float s = ps_r__Tree_SBC;
    RCache.tree.set_m_xform(xform); // matrix
    RCache.tree.set_consts(tvs.scale, tvs.scale, 0, 0); // consts/scale
    RCache.tree.set_wave(tvs.wave); // wave
    RCache.tree.set_wind(tvs.wind); // wind

    s *= 1.3333f;
    RCache.tree.set_c_scale(s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
    RCache.tree.set_c_bias(s * c_bias.rgb.x, s * c_bias.rgb.y, s * c_bias.rgb.z, s * c_bias.hemi); // bias

    RCache.tree.set_c_sun(s * c_scale.sun, s * c_bias.sun, 0, 0); // sun

    constexpr const char* strBendersPos{"benders_pos"};
    constexpr const char* strBendersSetup{"benders_setup"};

    RCache.set_c(strBendersSetup, Fvector4{ps_ssfx_int_grass_params_1.x, ps_ssfx_int_grass_params_1.y, ps_ssfx_int_grass_params_1.z, ps_r2_ls_flags_ext.test(SSFX_INTER_GRASS) ? ps_ssfx_grass_interactive.y : 0.f});

    if (ps_r2_ls_flags_ext.test(SSFX_INTER_GRASS))
    {
        Fvector4* c_grass{};
        RCache.get_ConstantDirect(strBendersPos, sizeof grass_shader_data.pos + sizeof grass_shader_data.dir, reinterpret_cast<void**>(&c_grass), nullptr, nullptr);
        if (c_grass)
        {
            std::memcpy(c_grass, &grass_shader_data.pos, sizeof grass_shader_data.pos);
            std::memcpy(c_grass + std::size(grass_shader_data.pos), &grass_shader_data.dir, sizeof grass_shader_data.dir);
        }
    }

    bool is_bugged_flora{};
    if (!ps_ssfx_wind_bugged_flora_enable)
    {
        if (const R_constant* C = &*RCache.get_c(CRender::c_sbase))
        {
            if (const CTexture* T = RCache.get_ActiveTexture(u32(C->samp.index)))
            {
                const char* tex = T->cName.c_str();
                is_bugged_flora = (strstr(tex, "trees\\trees_elka") || strstr(tex, "trees\\trees_kamysh"));
            }
        }
    }
    RCache.set_c("is_bugged_flora", static_cast<float>(is_bugged_flora));
}

#define PCOPY(a) a = pFrom->a
void FTreeVisual::Copy(dxRender_Visual* pSrc)
{
    dxRender_Visual::Copy(pSrc);

    FTreeVisual* pFrom = dynamic_cast<FTreeVisual*>(pSrc);

    PCOPY(rm_geom);

    PCOPY(p_rm_Vertices);
    if (p_rm_Vertices)
        p_rm_Vertices->AddRef();

    PCOPY(vBase);
    PCOPY(vCount);

    PCOPY(p_rm_Indices);
    if (p_rm_Indices)
        p_rm_Indices->AddRef();

    PCOPY(iBase);
    PCOPY(iCount);
    PCOPY(dwPrimitives);

    PCOPY(xform);
    PCOPY(c_scale);
    PCOPY(c_bias);
}

//-----------------------------------------------------------------------------------
// Stripified Tree
//-----------------------------------------------------------------------------------
FTreeVisual_ST::FTreeVisual_ST(void) {}
FTreeVisual_ST::~FTreeVisual_ST(void) {}
void FTreeVisual_ST::Release() { inherited::Release(); }
void FTreeVisual_ST::Load(const char* N, IReader* data, u32 dwFlags) { inherited::Load(N, data, dwFlags); }
void FTreeVisual_ST::Render(float LOD)
{
    inherited::Render(LOD);
    RCache.set_Geometry(rm_geom);
    RCache.Render(D3DPT_TRIANGLELIST, vBase, 0, vCount, iBase, dwPrimitives);
    RCache.stat.r.s_flora.add(vCount);
}
void FTreeVisual_ST::Copy(dxRender_Visual* pSrc) { inherited::Copy(pSrc); }

//-----------------------------------------------------------------------------------
// Progressive Tree
//-----------------------------------------------------------------------------------
FTreeVisual_PM::FTreeVisual_PM(void)
{
    pSWI = 0;
    last_lod = 0;
}
FTreeVisual_PM::~FTreeVisual_PM(void) {}
void FTreeVisual_PM::Release() { inherited::Release(); }
void FTreeVisual_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
    inherited::Load(N, data, dwFlags);
    R_ASSERT(data->find_chunk(OGF_SWICONTAINER));
    {
        u32 ID = data->r_u32();
        pSWI = RImplementation.getSWI(ID);
    }
}
void FTreeVisual_PM::Render(float LOD)
{
    inherited::Render(LOD);
    int lod_id = last_lod;
    if (LOD >= 0.f)
    {
        lod_id = iFloor((1.f - LOD) * float(pSWI->count - 1) + 0.5f);
        last_lod = lod_id;
    }
    VERIFY(lod_id >= 0 && lod_id < int(pSWI->count));
    FSlideWindow& SW = pSWI->sw[lod_id];
    RCache.set_Geometry(rm_geom);
    RCache.Render(D3DPT_TRIANGLELIST, vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris);
    RCache.stat.r.s_flora.add(SW.num_verts);
}
void FTreeVisual_PM::Copy(dxRender_Visual* pSrc)
{
    inherited::Copy(pSrc);
    FTreeVisual_PM* pFrom = dynamic_cast<FTreeVisual_PM*>(pSrc);
    PCOPY(pSWI);
}
