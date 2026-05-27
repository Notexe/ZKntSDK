#include <cr.h>
#include "ModSDK.hpp"
#include "HostServices.hpp"

CR_EXPORT int cr_main(cr_plugin* p_Ctx, cr_op operation) {
    switch (operation) {
    case CR_LOAD:
        zknt::ModSDK::GetInstance()->Startup(static_cast<knt::host::HostServices*>(p_Ctx ? p_Ctx->userdata : nullptr));
        return 0;
    case CR_UNLOAD:
    case CR_CLOSE:
        zknt::ModSDK::DestroyInstance();
        return 0;
    default:
        return 0;
    }
}
