/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define DIRECTINPUT_VERSION 0x0800

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"
#include "dinputd.h"

#include "wine/hid.h"

#include "dinput_test.h"

struct check_objects_todos
{
    BOOL type;
    BOOL guid;
    BOOL usage;
    BOOL name;
};

struct check_objects_params
{
    DWORD version;
    UINT index;
    UINT expect_count;
    const DIDEVICEOBJECTINSTANCEW *expect_objs;
    const struct check_objects_todos *todo_objs;
    BOOL todo_extra;
};

static BOOL CALLBACK check_objects( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    static const DIDEVICEOBJECTINSTANCEW unexpected_obj = {0};
    static const struct check_objects_todos todo_none = {0};
    struct check_objects_params *params = args;
    const DIDEVICEOBJECTINSTANCEW *exp = params->expect_objs + params->index;
    const struct check_objects_todos *todo;

    if (!params->todo_objs) todo = &todo_none;
    else todo = params->todo_objs + params->index;

    todo_wine_if( params->todo_extra && params->index >= params->expect_count )
    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) return DIENUM_STOP;

    winetest_push_context( "obj[%d]", params->index );

    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) exp = &unexpected_obj;

    check_member( *obj, *exp, "%u", dwSize );
    todo_wine_if( todo->guid )
    check_member_guid( *obj, *exp, guidType );
    todo_wine_if( params->version < 0x700 && (obj->dwType & DIDFT_BUTTON) )
    check_member( *obj, *exp, "%#x", dwOfs );
    todo_wine_if( todo->type )
    check_member( *obj, *exp, "%#x", dwType );
    check_member( *obj, *exp, "%#x", dwFlags );
    if (!localized) todo_wine_if( todo->name ) check_member_wstr( *obj, *exp, tszName );
    check_member( *obj, *exp, "%u", dwFFMaxForce );
    check_member( *obj, *exp, "%u", dwFFForceResolution );
    check_member( *obj, *exp, "%u", wCollectionNumber );
    check_member( *obj, *exp, "%u", wDesignatorIndex );
    check_member( *obj, *exp, "%#04x", wUsagePage );
    todo_wine_if( todo->usage )
    check_member( *obj, *exp, "%#04x", wUsage );
    check_member( *obj, *exp, "%#04x", dwDimension );
    check_member( *obj, *exp, "%#04x", wExponent );
    check_member( *obj, *exp, "%u", wReportId );

    winetest_pop_context();

    params->index++;
    return DIENUM_CONTINUE;
}

struct check_effects_params
{
    UINT index;
    UINT expect_count;
    const DIEFFECTINFOW *expect_effects;
};

static BOOL CALLBACK check_effects( const DIEFFECTINFOW *effect, void *args )
{
    static const DIEFFECTINFOW unexpected_effect = {0};
    struct check_effects_params *params = args;
    const DIEFFECTINFOW *exp = params->expect_effects + params->index;

    winetest_push_context( "effect[%d]", params->index );

    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) exp = &unexpected_effect;

    check_member( *effect, *exp, "%u", dwSize );
    check_member_guid( *effect, *exp, guid );
    check_member( *effect, *exp, "%#x", dwEffType );
    check_member( *effect, *exp, "%#x", dwStaticParams );
    check_member( *effect, *exp, "%#x", dwDynamicParams );
    check_member_wstr( *effect, *exp, tszName );

    winetest_pop_context();
    params->index++;

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_effect_count( const DIEFFECTINFOW *effect, void *args )
{
    DWORD *count = args;
    *count = *count + 1;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_no_created_effect_objects( IDirectInputEffect *effect, void *context )
{
    ok( 0, "unexpected effect %p\n", effect );
    return DIENUM_CONTINUE;
}

struct check_created_effect_params
{
    IDirectInputEffect *expect_effect;
    DWORD count;
};

static BOOL CALLBACK check_created_effect_objects( IDirectInputEffect *effect, void *context )
{
    struct check_created_effect_params *params = context;
    ULONG ref;

    ok( effect == params->expect_effect, "got effect %p, expected %p\n", effect, params->expect_effect );
    params->count++;

    IDirectInputEffect_AddRef( effect );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 1, "got ref %u, expected 1\n", ref );
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK enum_device_count( const DIDEVICEINSTANCEW *devinst, void *context )
{
    DWORD *count = context;
    *count = *count + 1;
    return DIENUM_CONTINUE;
}

static void check_dinput_devices( DWORD version, DIDEVICEINSTANCEW *devinst )
{
    IDirectInput8W *di8;
    IDirectInputW *di;
    ULONG count;
    HRESULT hr;

    if (version >= 0x800)
    {
        hr = DirectInput8Create( instance, version, &IID_IDirectInput8W, (void **)&di8, NULL );
        ok( hr == DI_OK, "DirectInput8Create returned %#x\n", hr );

        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, NULL, NULL, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, enum_device_count, &count, 0xdeadbeef );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        hr = IDirectInput8_EnumDevices( di8, 0xdeadbeef, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 3, "got count %u, expected 0\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_DEVICE, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 0, "got count %u, expected 0\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_POINTER, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        todo_wine
        ok( count == 3, "got count %u, expected 3\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_KEYBOARD, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        todo_wine
        ok( count == 3, "got count %u, expected 3\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_GAMECTRL, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 1, "got count %u, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 1, "got count %u, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_FORCEFEEDBACK );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        if (IsEqualGUID( &devinst->guidFFDriver, &GUID_NULL )) ok( count == 0, "got count %u, expected 0\n", count );
        else ok( count == 1, "got count %u, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff) + 1, enum_device_count, &count, DIEDFL_ALLDEVICES );
        if ((devinst->dwDevType & 0xff) != DI8DEVTYPE_SUPPLEMENTAL) ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        else ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        ok( count == 0, "got count %u, expected 0\n", count );
    }
    else
    {
        hr = DirectInputCreateEx( instance, version, &IID_IDirectInput2W, (void **)&di, NULL );
        ok( hr == DI_OK, "DirectInputCreateEx returned %#x\n", hr );

        hr = IDirectInput_EnumDevices( di, 0, NULL, NULL, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, 0xdeadbeef );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        hr = IDirectInput_EnumDevices( di, 0xdeadbeef, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, DIEDFL_INCLUDEHIDDEN );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );

        count = 0;
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 3, "got count %u, expected 0\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_DEVICE, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 0, "got count %u, expected 0\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_MOUSE, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        todo_wine
        ok( count == 3, "got count %u, expected 3\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_KEYBOARD, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        todo_wine
        ok( count == 3, "got count %u, expected 3\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_JOYSTICK, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 1, "got count %u, expected 1\n", count );

        count = 0;
        hr = IDirectInput_EnumDevices( di, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        ok( count == 1, "got count %u, expected 1\n", count );

        count = 0;
        hr = IDirectInput_EnumDevices( di, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_FORCEFEEDBACK );
        ok( hr == DI_OK, "EnumDevices returned: %#x\n", hr );
        if (IsEqualGUID( &devinst->guidFFDriver, &GUID_NULL )) ok( count == 0, "got count %u, expected 0\n", count );
        else ok( count == 1, "got count %u, expected 1\n", count );

        hr = IDirectInput_EnumDevices( di, 0x14, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#x\n", hr );
    }
}

static void test_periodic_effect( IDirectInputDevice8W *device, HANDLE file, DWORD version )
{
    struct hid_expect expect_download[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x01,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
        /* start command when DIEP_START is set */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {0x02,0x01,0x01,0x01},
        },
    };
    struct hid_expect expect_download_0[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x00},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x00,0x00,0x00,0x00,0x00,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_download_1[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_download_2[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
    };
    struct hid_expect expect_update[] =
    {
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0xff, 0xff, version >= 0x700 ? 0x06 : 0x00, 0x00, 0x01, 0x55, 0xd5},
        },
    };
    struct hid_expect expect_set_envelope[] =
    {
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06, 0x19, 0x4c, 0x01, 0x00, 0x04, 0x00},
        },
    };
    struct hid_expect expect_start =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x01},
    };
    struct hid_expect expect_start_solo =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x02, 0x01},
    };
    struct hid_expect expect_start_0 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x00},
    };
    struct hid_expect expect_start_4 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x04},
    };
    struct hid_expect expect_stop =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x03, 0x00},
    };
    struct hid_expect expect_unload[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {0x02,0x01,0x03,0x00},
        },
        /* device reset, when unloaded from Unacquire */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1,0x01},
        },
    };
    struct hid_expect expect_acquire[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0x19},
        },
    };
    struct hid_expect expect_reset[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    static const DWORD expect_axes_init[2] = {0};
    const DIEFFECT expect_desc_init =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwTriggerButton = -1,
        .rgdwAxes = (void *)expect_axes_init,
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] =
    {
        +3000,
        -6000,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DIPERIODIC expect_periodic =
    {
        .dwMagnitude = 1000,
        .lOffset = 2000,
        .dwPhase = 3000,
        .dwPeriod = 4000,
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 3,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = sizeof(DIPERIODIC),
        .lpvTypeSpecificParams = (void *)&expect_periodic,
        .dwStartDelay = 6000,
    };
    static const LONG expect_directions_0[3] = {0};
    static const DIENVELOPE expect_envelope_0 = {.dwSize = sizeof(DIENVELOPE)};
    static const DIPERIODIC expect_periodic_0 = {0};
    const DIEFFECT expect_desc_0 =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .cAxes = 3,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions_0,
        .lpEnvelope = (void *)&expect_envelope_0,
        .cbTypeSpecificParams = sizeof(DIPERIODIC),
        .lpvTypeSpecificParams = (void *)&expect_periodic_0,
    };
    struct check_created_effect_params check_params = {0};
    IDirectInputEffect *effect;
    DIPERIODIC periodic = {0};
    DIENVELOPE envelope = {0};
    LONG directions[4] = {0};
    DIEFFECT desc = {0};
    DWORD axes[4] = {0};
    ULONG i, ref, flags;
    HRESULT hr;
    GUID guid;

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, NULL, NULL );
    ok( hr == E_POINTER, "CreateEffect returned %#x\n", hr );
    hr = IDirectInputDevice8_CreateEffect( device, NULL, NULL, &effect, NULL );
    ok( hr == E_POINTER, "CreateEffect returned %#x\n", hr );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_NULL, NULL, &effect, NULL );
    ok( hr == DIERR_DEVICENOTREG, "CreateEffect returned %#x\n", hr );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    if (hr != DI_OK) return;

    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_no_created_effect_objects, effect, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "EnumCreatedEffectObjects returned %#x\n", hr );
    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#x\n", hr );
    ok( check_params.count == 1, "got count %u, expected 1\n", check_params.count );

    hr = IDirectInputEffect_Initialize( effect, NULL, version, &GUID_Sine );
    ok( hr == DIERR_INVALIDPARAM, "Initialize returned %#x\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, 0x800 - (version - 0x700), &GUID_Sine );
    if (version == 0x800)
    {
        todo_wine
        ok( hr == DIERR_BETADIRECTINPUTVERSION, "Initialize returned %#x\n", hr );
    }
    else
    {
        todo_wine
        ok( hr == DIERR_OLDDIRECTINPUTVERSION, "Initialize returned %#x\n", hr );
    }
    hr = IDirectInputEffect_Initialize( effect, instance, 0, &GUID_Sine );
    todo_wine
    ok( hr == DIERR_NOTINITIALIZED, "Initialize returned %#x\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, NULL );
    ok( hr == E_POINTER, "Initialize returned %#x\n", hr );

    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_NULL );
    ok( hr == DIERR_DEVICENOTREG, "Initialize returned %#x\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_Sine );
    ok( hr == DI_OK, "Initialize returned %#x\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_Square );
    ok( hr == DI_OK, "Initialize returned %#x\n", hr );

    hr = IDirectInputEffect_GetEffectGuid( effect, NULL );
    ok( hr == E_POINTER, "GetEffectGuid returned %#x\n", hr );
    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#x\n", hr );
    ok( IsEqualGUID( &guid, &GUID_Square ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_Square ) );

    hr = IDirectInputEffect_GetParameters( effect, NULL, 0 );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, NULL, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX5) + 2;
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX5);
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_STARTDELAY );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX6);
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    desc.dwDuration = 0xdeadbeef;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", dwDuration );
    memset( &desc, 0xcd, sizeof(desc) );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    desc.dwFlags = 0;
    desc.dwStartDelay = 0xdeadbeef;
    flags = DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL |
            (version >= 0x700 ? DIEP_STARTDELAY : 0);
    hr = IDirectInputEffect_GetParameters( effect, &desc, flags );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%u", dwGain );
    if (version >= 0x700) check_member( desc, expect_desc_init, "%u", dwStartDelay );
    else ok( desc.dwStartDelay == 0xdeadbeef, "got dwStartDelay %#x\n", desc.dwStartDelay );
    check_member( desc, expect_desc_init, "%u", dwTriggerRepeatInterval );

    memset( &desc, 0xcd, sizeof(desc) );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    desc.dwFlags = 0;
    desc.lpEnvelope = NULL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == E_POINTER, "GetParameters returned %#x\n", hr );
    desc.lpEnvelope = &envelope;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    envelope.dwSize = sizeof(DIENVELOPE);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );

    desc.dwFlags = 0;
    desc.cAxes = 0;
    desc.rgdwAxes = NULL;
    desc.rglDirection = NULL;
    desc.lpEnvelope = NULL;
    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = NULL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%#x", dwTriggerButton );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", cAxes );
    desc.dwFlags = DIEFF_OBJECTIDS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%#x", dwTriggerButton );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", cAxes );
    desc.dwFlags |= DIEFF_CARTESIAN;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );
    desc.dwFlags |= DIEFF_POLAR;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );
    desc.dwFlags |= DIEFF_SPHERICAL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", cAxes );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );

    desc.dwFlags |= DIEFF_SPHERICAL;
    desc.cAxes = 2;
    desc.rgdwAxes = axes;
    desc.rglDirection = directions;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", cAxes );
    check_member( desc, expect_desc_init, "%u", rgdwAxes[0] );
    check_member( desc, expect_desc_init, "%u", rgdwAxes[1] );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );

    desc.dwFlags |= DIEFF_SPHERICAL;
    desc.lpEnvelope = &envelope;
    desc.cbTypeSpecificParams = sizeof(periodic);
    desc.lpvTypeSpecificParams = &periodic;
    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc_init, "%u", dwDuration );
    check_member( desc, expect_desc_init, "%u", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%u", dwGain );
    check_member( desc, expect_desc_init, "%#x", dwTriggerButton );
    check_member( desc, expect_desc_init, "%u", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%u", cAxes );
    check_member( desc, expect_desc_init, "%u", rgdwAxes[0] );
    check_member( desc, expect_desc_init, "%u", rgdwAxes[1] );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%u", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc_init, "%u", dwStartDelay );
    else ok( desc.dwStartDelay == 0xcdcdcdcd, "got dwStartDelay %#x\n", desc.dwStartDelay );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Download returned %#x\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, NULL, DIEP_NODOWNLOAD );
    ok( hr == E_POINTER, "SetParameters returned %#x\n", hr );
    memset( &desc, 0, sizeof(desc) );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DURATION );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DURATION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    desc.dwTriggerButton = -1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", dwDuration );
    check_member( desc, expect_desc_init, "%u", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%u", dwGain );
    check_member( desc, expect_desc_init, "%#x", dwTriggerButton );
    check_member( desc, expect_desc_init, "%u", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%u", cAxes );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%u", cbTypeSpecificParams );
    check_member( desc, expect_desc_init, "%u", dwStartDelay );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    flags = DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL | DIEP_NODOWNLOAD;
    if (version >= 0x700) flags |= DIEP_STARTDELAY;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.dwDuration = 0;
    flags = DIEP_DURATION | DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL;
    if (version >= 0x700) flags |= DIEP_STARTDELAY;
    hr = IDirectInputEffect_GetParameters( effect, &desc, flags );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", dwDuration );
    check_member( desc, expect_desc, "%u", dwSamplePeriod );
    check_member( desc, expect_desc, "%u", dwGain );
    check_member( desc, expect_desc_init, "%#x", dwTriggerButton );
    check_member( desc, expect_desc, "%u", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%u", cAxes );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%u", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc, "%u", dwStartDelay );
    else ok( desc.dwStartDelay == 0, "got dwStartDelay %#x\n", desc.dwStartDelay );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    desc.lpEnvelope = NULL;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.lpEnvelope = &envelope;
    envelope.dwSize = 0;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    desc.lpEnvelope = &envelope;
    envelope.dwSize = sizeof(DIENVELOPE);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( envelope, expect_envelope, "%u", dwAttackLevel );
    check_member( envelope, expect_envelope, "%u", dwAttackTime );
    check_member( envelope, expect_envelope, "%u", dwFadeLevel );
    check_member( envelope, expect_envelope, "%u", dwFadeTime );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    desc.dwFlags = 0;
    desc.cAxes = 0;
    desc.rgdwAxes = NULL;
    desc.rglDirection = NULL;
    desc.lpEnvelope = NULL;
    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = NULL;
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &desc, flags | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );

    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    desc.cAxes = 1;
    desc.rgdwAxes = axes;
    desc.rgdwAxes[0] = DIJOFS_X;
    desc.dwTriggerButton = DIJOFS_BUTTON( 1 );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_AXES | DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DIERR_ALREADYINITIALIZED, "SetParameters returned %#x\n", hr );

    desc.cAxes = 0;
    desc.dwFlags = DIEFF_OBJECTIDS;
    desc.rgdwAxes = axes;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%#x", dwTriggerButton );
    check_member( desc, expect_desc, "%u", cAxes );
    check_member( desc, expect_desc, "%u", rgdwAxes[0] );
    check_member( desc, expect_desc, "%u", rgdwAxes[1] );
    check_member( desc, expect_desc, "%u", rgdwAxes[2] );

    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    ok( desc.dwTriggerButton == 0x30, "got %#x expected %#x\n", desc.dwTriggerButton, 0x30 );
    ok( desc.rgdwAxes[0] == 8, "got %#x expected %#x\n", desc.rgdwAxes[0], 8 );
    ok( desc.rgdwAxes[1] == 0, "got %#x expected %#x\n", desc.rgdwAxes[1], 0 );
    ok( desc.rgdwAxes[2] == 4, "got %#x expected %#x\n", desc.rgdwAxes[2], 4 );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 0;
    desc.rglDirection = directions;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    desc.cAxes = 3;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 3;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    desc.dwFlags = DIEFF_SPHERICAL;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_SPHERICAL, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_SPHERICAL );
    check_member( desc, expect_desc, "%u", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", cAxes );
    ok( desc.rglDirection[0] == 3000, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 3000 );
    ok( desc.rglDirection[1] == 30000, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], 30000 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %d expected %d\n", desc.rglDirection[2], 0 );
    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 2;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_CARTESIAN, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_CARTESIAN );
    check_member( desc, expect_desc, "%u", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", cAxes );
    ok( desc.rglDirection[0] == 4330, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 4330 );
    ok( desc.rglDirection[1] == 2500, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], 2500 );
    ok( desc.rglDirection[2] == -8660, "got rglDirection[2] %d expected %d\n", desc.rglDirection[2], -8660 );
    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 3;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#x\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = &periodic;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
    desc.cbTypeSpecificParams = sizeof(DIPERIODIC);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( periodic, expect_periodic, "%u", dwMagnitude );
    check_member( periodic, expect_periodic, "%d", lOffset );
    check_member( periodic, expect_periodic, "%u", dwPhase );
    check_member( periodic, expect_periodic, "%u", dwPeriod );

    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DIERR_NOTDOWNLOADED, "Start returned %#x\n", hr );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DIERR_NOTDOWNLOADED, "Stop returned %#x\n", hr );

    set_hid_expect( file, expect_download, 3 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_NOEFFECT, "Download returned %#x\n", hr );

    hr = IDirectInputEffect_Start( effect, 1, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "Start returned %#x\n", hr );

    set_hid_expect( file, &expect_start_solo, sizeof(expect_start_solo) );
    hr = IDirectInputEffect_Start( effect, 1, DIES_SOLO );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start_4, sizeof(expect_start_4) );
    hr = IDirectInputEffect_Start( effect, 4, 0 );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start_0, sizeof(expect_start_4) );
    hr = IDirectInputEffect_Start( effect, 0, 0 );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_download, 4 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_START );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_download, 3 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, 2 * sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Start returned %#x\n", hr );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Stop returned %#x\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#x\n", hr );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );

    desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTIDS;
    desc.cAxes = 2;
    desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
    desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR;
    desc.rglDirection[0] = 3000;
    desc.rglDirection[1] = 0;
    desc.rglDirection[2] = 0;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.rglDirection[0] = 0;

    desc.dwFlags = DIEFF_SPHERICAL;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_SPHERICAL, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_SPHERICAL );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 30000, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 30000 );
    ok( desc.rglDirection[1] == 0, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], 0 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %d expected %d\n", desc.rglDirection[2], 0 );

    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_CARTESIAN, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_CARTESIAN );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 5000, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 5000 );
    ok( desc.rglDirection[1] == -8660, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], -8660 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %d expected %d\n", desc.rglDirection[2], 0 );

    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
    ok( desc.dwFlags == DIEFF_POLAR, "got flags %#x, expected %#x\n", desc.dwFlags, DIEFF_POLAR );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    ok( desc.cAxes == 2, "got cAxes %u expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 3000, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 3000 );
    ok( desc.rglDirection[1] == 0, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], 0 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %d expected %d\n", desc.rglDirection[2], 0 );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );

    for (i = 1; i < 4; i++)
    {
        struct hid_expect expect_directions[] =
        {
            /* set periodic */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 5,
                .report_len = 2,
                .report_buf = {0x05,0x19},
            },
            /* set envelope */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 6,
                .report_len = 7,
                .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
            },
            /* update effect */
            {0},
            /* effect control */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 2,
                .report_len = 4,
                .report_buf = {0x02,0x01,0x03,0x00},
            },
        };
        struct hid_expect expect_spherical =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = { 0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00, 0x01,
                            i >= 2 ? 0x55 : 0, i >= 3 ? 0x1c : 0 },
        };
        struct hid_expect expect_cartesian =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00,
                           0x01, i >= 2 ? 0x63 : 0, i >= 3 ? 0x1d : 0},
        };
        struct hid_expect expect_polar =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00,
                           0x01, i >= 2 ? 0x3f : 0, i >= 3 ? 0x00 : 0},
        };

        winetest_push_context( "%u axes", i );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );

        desc.dwFlags = DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
        desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR;
        desc.rgdwAxes[2] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR;
        desc.rglDirection[0] = 0;
        desc.rglDirection[1] = 0;
        desc.rglDirection[2] = 0;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_NODOWNLOAD );
        ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

        desc.dwFlags = DIEFF_CARTESIAN;
        desc.cAxes = i == 3 ? 2 : 3;
        desc.rglDirection[0] = 1000;
        desc.rglDirection[1] = 2000;
        desc.rglDirection[2] = 3000;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
        ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#x\n", hr );
        desc.cAxes = i;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
        ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

        desc.dwFlags = DIEFF_SPHERICAL;
        desc.cAxes = i;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
        ok( desc.cAxes == i, "got cAxes %u expected 2\n", desc.cAxes );
        if (i == 1)
        {
            ok( desc.rglDirection[0] == 0, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 0 );
            ok( desc.rglDirection[1] == 0xcdcdcdcd, "got rglDirection[1] %d expected %d\n",
                desc.rglDirection[1], 0xcdcdcdcd );
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %d expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        }
        else
        {
            ok( desc.rglDirection[0] == 6343, "got rglDirection[0] %d expected %d\n",
                desc.rglDirection[0], 6343 );
            if (i == 2)
            {
                ok( desc.rglDirection[1] == 0, "got rglDirection[1] %d expected %d\n",
                    desc.rglDirection[1], 0 );
                ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %d expected %d\n",
                    desc.rglDirection[2], 0xcdcdcdcd );
            }
            else
            {
                ok( desc.rglDirection[1] == 5330, "got rglDirection[1] %d expected %d\n",
                    desc.rglDirection[1], 5330 );
                ok( desc.rglDirection[2] == 0, "got rglDirection[2] %d expected %d\n",
                    desc.rglDirection[2], 0 );
            }
        }

        desc.dwFlags = DIEFF_CARTESIAN;
        desc.cAxes = i;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
        ok( desc.cAxes == i, "got cAxes %u expected 2\n", desc.cAxes );
        ok( desc.rglDirection[0] == 1000, "got rglDirection[0] %d expected %d\n", desc.rglDirection[0], 1000 );
        if (i == 1)
            ok( desc.rglDirection[1] == 0xcdcdcdcd, "got rglDirection[1] %d expected %d\n",
                desc.rglDirection[1], 0xcdcdcdcd );
        else
            ok( desc.rglDirection[1] == 2000, "got rglDirection[1] %d expected %d\n",
                desc.rglDirection[1], 2000 );
        if (i <= 2)
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %d expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        else
            ok( desc.rglDirection[2] == 3000, "got rglDirection[2] %d expected %d\n",
                desc.rglDirection[2], 3000 );

        desc.dwFlags = DIEFF_POLAR;
        desc.cAxes = 1;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        if (i != 2) ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
        else ok( hr == DIERR_MOREDATA, "GetParameters returned %#x\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        if (i != 2) ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#x\n", hr );
        else
        {
            ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
            ok( desc.cAxes == i, "got cAxes %u expected 2\n", desc.cAxes );
            ok( desc.rglDirection[0] == 15343, "got rglDirection[0] %d expected %d\n",
                desc.rglDirection[0], 15343 );
            ok( desc.rglDirection[1] == 0, "got rglDirection[1] %d expected %d\n", desc.rglDirection[1], 0 );
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %d expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        }

        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %d\n", ref );

        desc = expect_desc;
        desc.dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes = axes;
        desc.rglDirection = directions;
        desc.rglDirection[0] = 3000;
        desc.rglDirection[1] = 4000;
        desc.rglDirection[2] = 5000;
        flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
        expect_directions[2] = expect_spherical;
        set_hid_expect( file, expect_directions, sizeof(expect_directions) );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %d\n", ref );
        set_hid_expect( file, NULL, 0 );

        desc = expect_desc;
        desc.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes = axes;
        desc.rglDirection = directions;
        desc.rglDirection[0] = 6000;
        desc.rglDirection[1] = 7000;
        desc.rglDirection[2] = 8000;
        flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
        expect_directions[2] = expect_cartesian;
        set_hid_expect( file, expect_directions, sizeof(expect_directions) );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %d\n", ref );
        set_hid_expect( file, NULL, 0 );

        if (i == 2)
        {
            desc = expect_desc;
            desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTIDS;
            desc.cAxes = i;
            desc.rgdwAxes = axes;
            desc.rglDirection = directions;
            desc.rglDirection[0] = 9000;
            desc.rglDirection[1] = 10000;
            desc.rglDirection[2] = 11000;
            flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
            expect_directions[2] = expect_polar;
            set_hid_expect( file, expect_directions, sizeof(expect_directions) );
            hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
            ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
            ref = IDirectInputEffect_Release( effect );
            ok( ref == 0, "Release returned %d\n", ref );
            set_hid_expect( file, NULL, 0 );
        }

        winetest_pop_context();
    }

    /* zero-ed effect parameters are sent */

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );

    set_hid_expect( file, expect_download_0, sizeof(expect_download_0) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc_0, flags );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

    set_hid_expect( file, expect_download_1, sizeof(expect_download_1) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc_0, (flags & ~DIEP_ENVELOPE) );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );


    set_hid_expect( file, expect_download_2, sizeof(expect_download_2) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    desc = expect_desc;
    desc.dwDuration = INFINITE;
    desc.dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_DURATION | DIEP_TRIGGERBUTTON );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, expect_update, sizeof(expect_update) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */
    desc = expect_desc;
    desc.dwDuration = INFINITE;
    desc.dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_DURATION | DIEP_TRIGGERBUTTON );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, expect_update, sizeof(expect_update) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    desc = expect_desc;
    desc.lpEnvelope = &envelope;
    desc.lpEnvelope->dwAttackTime = 1000;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_ENVELOPE );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    set_hid_expect( file, expect_set_envelope, sizeof(expect_set_envelope) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */
}

static void test_condition_effect( IDirectInputDevice8W *device, HANDLE file, DWORD version )
{
    struct hid_expect expect_create[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_1[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_create_2[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xf1},
        },
    };
    struct hid_expect expect_create_3[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_destroy =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x03, 0x00},
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] = {
        +3000,
        0,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONDITION expect_condition[3] =
    {
        {
            .lOffset = -500,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = -4000,
            .dwNegativeSaturation = -5000,
            .lDeadBand = 6000,
        },
        {
            .lOffset = 6000,
            .lPositiveCoefficient = 5000,
            .lNegativeCoefficient = -4000,
            .dwPositiveSaturation = 3000,
            .dwNegativeSaturation = 2000,
            .lDeadBand = 1000,
        },
        {
            .lOffset = -7000,
            .lPositiveCoefficient = -8000,
            .lNegativeCoefficient = 9000,
            .dwPositiveSaturation = 10000,
            .dwNegativeSaturation = 11000,
            .lDeadBand = -12000,
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 2,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = (void *)expect_condition,
        .dwStartDelay = 6000,
    };
    struct check_created_effect_params check_params = {0};
    DIENVELOPE envelope = {.dwSize = sizeof(DIENVELOPE)};
    DICONDITION condition[2] = {0};
    IDirectInputEffect *effect;
    LONG directions[4] = {0};
    DWORD axes[4] = {0};
    DIEFFECT desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .cAxes = 4,
        .rgdwAxes = axes,
        .rglDirection = directions,
        .lpEnvelope = &envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = condition,
    };
    HRESULT hr;
    ULONG ref;
    GUID guid;

    set_hid_expect( file, expect_create, sizeof(expect_create) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#x\n", hr );
    ok( check_params.count == 1, "got count %u, expected 1\n", check_params.count );

    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#x\n", hr );
    ok( IsEqualGUID( &guid, &GUID_Spring ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_Spring ) );

    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DI_OK, "GetParameters returned %#x\n", hr );
    check_member( desc, expect_desc, "%u", dwDuration );
    check_member( desc, expect_desc, "%u", dwSamplePeriod );
    check_member( desc, expect_desc, "%u", dwGain );
    check_member( desc, expect_desc, "%#x", dwTriggerButton );
    check_member( desc, expect_desc, "%u", dwTriggerRepeatInterval );
    check_member( desc, expect_desc, "%u", cAxes );
    check_member( desc, expect_desc, "%#x", rgdwAxes[0] );
    check_member( desc, expect_desc, "%#x", rgdwAxes[1] );
    check_member( desc, expect_desc, "%d", rglDirection[0] );
    check_member( desc, expect_desc, "%d", rglDirection[1] );
    check_member( desc, expect_desc, "%u", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc, "%u", dwStartDelay );
    else ok( desc.dwStartDelay == 0, "got dwStartDelay %#x\n", desc.dwStartDelay );
    check_member( envelope, expect_envelope, "%u", dwAttackLevel );
    check_member( envelope, expect_envelope, "%u", dwAttackTime );
    check_member( envelope, expect_envelope, "%u", dwFadeLevel );
    check_member( envelope, expect_envelope, "%u", dwFadeTime );
    check_member( condition[0], expect_condition[0], "%d", lOffset );
    check_member( condition[0], expect_condition[0], "%d", lPositiveCoefficient );
    check_member( condition[0], expect_condition[0], "%d", lNegativeCoefficient );
    check_member( condition[0], expect_condition[0], "%u", dwPositiveSaturation );
    check_member( condition[0], expect_condition[0], "%u", dwNegativeSaturation );
    check_member( condition[0], expect_condition[0], "%d", lDeadBand );
    check_member( condition[1], expect_condition[1], "%d", lOffset );
    check_member( condition[1], expect_condition[1], "%d", lPositiveCoefficient );
    check_member( condition[1], expect_condition[1], "%d", lNegativeCoefficient );
    check_member( condition[1], expect_condition[1], "%u", dwPositiveSaturation );
    check_member( condition[1], expect_condition[1], "%u", dwNegativeSaturation );
    check_member( condition[1], expect_condition[1], "%d", lDeadBand );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 1;
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DIERR_INVALIDPARAM, "CreateEffect returned %#x\n", hr );
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_1, sizeof(expect_create_1) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 3;
    desc.rglDirection = directions;
    desc.rglDirection[0] = +3000;
    desc.rglDirection[1] = -2000;
    desc.rglDirection[2] = +1000;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_2, sizeof(expect_create_2) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 2;
    desc.rgdwAxes = axes;
    desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR;
    desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
    desc.rglDirection = directions;
    desc.rglDirection[0] = +3000;
    desc.rglDirection[1] = -2000;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_3, sizeof(expect_create_3) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    desc = expect_desc;
    desc.cAxes = 0;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[0];
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DIERR_MOREDATA, "SetParameters returned %#x\n", hr );
    ok( desc.cbTypeSpecificParams == 1 * sizeof(DICONDITION), "got %u\n", desc.cbTypeSpecificParams );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DI_OK, "SetParameters returned %#x\n", hr );
    ok( desc.cbTypeSpecificParams == 0 * sizeof(DICONDITION), "got %u\n", desc.cbTypeSpecificParams );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
}

static BOOL test_force_feedback_joystick( DWORD version )
{
#include "psh_hid_macros.h"
    const unsigned char report_descriptor[] = {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 6),
                INPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MAXIMUM(1, 0x7f),
                LOGICAL_MINIMUM(1, 0x00),
                REPORT_SIZE(1, 7),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 2),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 2),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Z),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 3),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 4),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(2, 0x00ff),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 0x00008ca0),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_PERIODIC_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_ENVELOPE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_ATTACK_LEVEL),
                USAGE(1, PID_USAGE_FADE_LEVEL),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_ATTACK_TIME),
                USAGE(1, PID_USAGE_FADE_TIME),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                PHYSICAL_MAXIMUM(1, 0),
                UNIT_EXPONENT(1, 0),
                UNIT(1, 0),
            END_COLLECTION,


            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 7),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_CP_OFFSET),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,


            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 8),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
#undef REPORT_ID_OR_USAGE_PAGE
#include "pop_hid_macros.h"

    static const HIDP_CAPS hid_caps =
    {
        .InputReportByteLength = 5,
    };
    const DIDEVCAPS expect_caps =
    {
        .dwSize = sizeof(DIDEVCAPS),
        .dwFlags = DIDC_FORCEFEEDBACK | DIDC_ATTACHED | DIDC_EMULATED | DIDC_STARTDELAY |
                   DIDC_FFFADE | DIDC_FFATTACK | DIDC_DEADBAND | DIDC_SATURATION,
        .dwDevType = version >= 0x800 ? DIDEVTYPE_HID | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DI8DEVTYPE_JOYSTICK
                                      : DIDEVTYPE_HID | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_JOYSTICK,
        .dwAxes = 3,
        .dwButtons = 2,
        .dwFFSamplePeriod = 1000000,
        .dwFFMinTimeResolution = 1000000,
        .dwHardwareRevision = 1,
        .dwFFDriverVersion = 1,
    };
    struct hid_expect expect_acquire[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0x19},
        },
    };
    struct hid_expect expect_reset[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    struct hid_expect expect_set_device_gain_1 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 8,
        .report_len = 2,
        .report_buf = {8, 0x19},
    };
    struct hid_expect expect_set_device_gain_2 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 8,
        .report_len = 2,
        .report_buf = {8, 0x33},
    };

    const DIDEVICEINSTANCEW expect_devinst =
    {
        .dwSize = sizeof(DIDEVICEINSTANCEW),
        .guidInstance = expect_guid_product,
        .guidProduct = expect_guid_product,
        .dwDevType = version >= 0x800 ? DIDEVTYPE_HID | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DI8DEVTYPE_JOYSTICK
                                      : DIDEVTYPE_HID | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_JOYSTICK,
        .tszInstanceName = L"Wine test root driver",
        .tszProductName = L"Wine test root driver",
        .guidFFDriver = IID_IDirectInputPIDDriver,
        .wUsagePage = HID_USAGE_PAGE_GENERIC,
        .wUsage = HID_USAGE_GENERIC_JOYSTICK,
    };
    const DIDEVICEOBJECTINSTANCEW expect_objects_5[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(0)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"X Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = 0x4,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(1)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Y Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwOfs = 0x8,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(2)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Z Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0x30,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(0)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 0",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x1,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0x31,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(1)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 1",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x2,
            .wReportId = 1,
        },
    };
    const DIDEVICEOBJECTINSTANCEW expect_objects[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(2)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Z Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = 0x4,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(1)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Y Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwOfs = 0x8,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(0)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"X Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = version >= 0x800 ? 0x68 : 0x10,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(0)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 0",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x1,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = version >= 0x800 ? 0x69 : 0x11,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(1)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 1",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x2,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x70 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(12)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"DC Device Reset",
            .wCollectionNumber = 4,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DC_DEVICE_RESET,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x10 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(13)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Effect Block Index",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_BLOCK_INDEX,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x71 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(14)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Start",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_START,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x72 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(15)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Start Solo",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_START_SOLO,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x73 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(16)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Stop",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_STOP,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x14 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(17)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Loop Count",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_LOOP_COUNT,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x18 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(18)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Effect Block Index",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_BLOCK_INDEX,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x74 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(19)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Square",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SQUARE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x75 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(20)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Sine",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SINE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x76 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(21)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Spring",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SPRING,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x77 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(22)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Z Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x78 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(23)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Y Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x79 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(24)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"X Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7a : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(25)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Direction Enable",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DIRECTION_ENABLE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x1c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(26)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Start Delay",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_START_DELAY,
            .wReportId = 3,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x20 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(27)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Duration",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DURATION,
            .wReportId = 3,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x24 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(28)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Trigger Button",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_TRIGGER_BUTTON,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x28 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(29)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 29",
            .wCollectionNumber = 10,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 2,
            .wReportId = 3,
            .wExponent = -2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x2c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(30)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 30",
            .wCollectionNumber = 10,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 1,
            .wReportId = 3,
            .wExponent = -2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x30 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(31)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Magnitude",
            .wCollectionNumber = 11,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_MAGNITUDE,
            .wReportId = 5,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x34 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(32)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Fade Level",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_FADE_LEVEL,
            .wReportId = 6,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x38 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(33)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Attack Level",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ATTACK_LEVEL,
            .wReportId = 6,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x3c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(34)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Fade Time",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_FADE_TIME,
            .wReportId = 6,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x40 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(35)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Attack Time",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ATTACK_TIME,
            .wReportId = 6,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x44 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(36)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 36",
            .wCollectionNumber = 14,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 2,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x48 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(37)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 37",
            .wCollectionNumber = 14,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 1,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x4c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(38)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"CP Offset",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_CP_OFFSET,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x50 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(39)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Negative Coefficient",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_NEGATIVE_COEFFICIENT,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x54 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(40)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Positive Coefficient",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_POSITIVE_COEFFICIENT,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x58 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(41)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Negative Saturation",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_NEGATIVE_SATURATION,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x5c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(42)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Positive Saturation",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_POSITIVE_SATURATION,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x60 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(43)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Dead Band",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEAD_BAND,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x64 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(44)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Device Gain",
            .wCollectionNumber = 15,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_GAIN,
            .wReportId = 8,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(0),
            .tszName = L"Collection 0 - Joystick",
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_JOYSTICK,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(1),
            .tszName = L"Collection 1 - Joystick",
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_JOYSTICK,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(2),
            .tszName = L"Collection 2 - PID State Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_STATE_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(3),
            .tszName = L"Collection 3 - PID Device Control Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_CONTROL_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(4),
            .tszName = L"Collection 4 - PID Device Control",
            .wCollectionNumber = 3,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_CONTROL,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(5),
            .tszName = L"Collection 5 - Effect Operation Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_OPERATION_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(6),
            .tszName = L"Collection 6 - Effect Operation",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_OPERATION,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(7),
            .tszName = L"Collection 7 - Set Effect Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_EFFECT_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(8),
            .tszName = L"Collection 8 - Effect Type",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_TYPE,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(9),
            .tszName = L"Collection 9 - Axes Enable",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_AXES_ENABLE,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(10),
            .tszName = L"Collection 10 - Direction",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DIRECTION,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(11),
            .tszName = L"Collection 11 - Set Periodic Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_PERIODIC_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(12),
            .tszName = L"Collection 12 - Set Envelope Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_ENVELOPE_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(13),
            .tszName = L"Collection 13 - Set Condition Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_CONDITION_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(14),
            .tszName = L"Collection 14 - Type Specific Block Offset",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(15),
            .tszName = L"Collection 15 - Device Gain Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_GAIN_REPORT,
        },
    };
    const DIEFFECTINFOW expect_effects[] =
    {
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Square,
            .dwEffType = DIEFT_PERIODIC | DIEFT_STARTDELAY | DIEFT_FFFADE | DIEFT_FFATTACK,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .tszName = L"GUID_Square",
        },
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Sine,
            .dwEffType = DIEFT_PERIODIC | DIEFT_STARTDELAY | DIEFT_FFFADE | DIEFT_FFATTACK,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .tszName = L"GUID_Sine",
        },
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Spring,
            .dwEffType = DIEFT_CONDITION | DIEFT_STARTDELAY | DIEFT_DEADBAND | DIEFT_SATURATION,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION,
            .tszName = L"GUID_Spring",
        }
    };

    struct check_objects_todos todo_objects_5[ARRAY_SIZE(expect_objects_5)] =
    {
        {.guid = TRUE, .type = TRUE, .usage = TRUE, .name = TRUE},
        {0},
        {.guid = TRUE, .type = TRUE, .usage = TRUE, .name = TRUE},
    };
    struct check_objects_params check_objects_params =
    {
        .version = version,
        .expect_count = version < 0x700 ? ARRAY_SIZE(expect_objects_5) : ARRAY_SIZE(expect_objects),
        .expect_objs = version < 0x700 ? expect_objects_5 : expect_objects,
        .todo_objs = version < 0x700 ? todo_objects_5 : NULL,
        .todo_extra = version < 0x700 ? TRUE : FALSE,
    };
    struct check_effects_params check_effects_params =
    {
        .expect_count = ARRAY_SIZE(expect_effects),
        .expect_effects = expect_effects,
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    WCHAR cwd[MAX_PATH], tempdir[MAX_PATH];
    IDirectInputDevice8W *device = NULL;
    DIDEVICEOBJECTDATA objdata = {0};
    DIEFFECTINFOW effectinfo = {0};
    DIEFFESCAPE escape = {0};
    DIDEVCAPS caps = {0};
    char buffer[1024];
    ULONG res, ref;
    HANDLE file;
    HRESULT hr;
    HWND hwnd;

    winetest_push_context( "%#x", version );

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    cleanup_registry_keys();
    if (!dinput_driver_start( report_descriptor, sizeof(report_descriptor), &hid_caps, NULL, 0 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    check_dinput_devices( version, &devinst );

    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#x\n", hr );
    check_member( devinst, expect_devinst, "%d", dwSize );
    todo_wine
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    check_member( devinst, expect_devinst, "%#x", dwDevType );
    todo_wine
    check_member_wstr( devinst, expect_devinst, tszInstanceName );
    todo_wine
    check_member_wstr( devinst, expect_devinst, tszProductName );
    check_member_guid( devinst, expect_devinst, guidFFDriver );
    check_member( devinst, expect_devinst, "%04x", wUsagePage );
    check_member( devinst, expect_devinst, "%04x", wUsage );

    caps.dwSize = sizeof(DIDEVCAPS);
    hr = IDirectInputDevice8_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetCapabilities returned %#x\n", hr );
    check_member( caps, expect_caps, "%d", dwSize );
    check_member( caps, expect_caps, "%#x", dwFlags );
    check_member( caps, expect_caps, "%#x", dwDevType );
    check_member( caps, expect_caps, "%d", dwAxes );
    check_member( caps, expect_caps, "%d", dwButtons );
    check_member( caps, expect_caps, "%d", dwPOVs );
    check_member( caps, expect_caps, "%d", dwFFSamplePeriod );
    check_member( caps, expect_caps, "%d", dwFFMinTimeResolution );
    check_member( caps, expect_caps, "%d", dwFirmwareRevision );
    check_member( caps, expect_caps, "%d", dwHardwareRevision );
    check_member( caps, expect_caps, "%d", dwFFDriverVersion );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFGAIN returned %#x\n", hr );
    ok( prop_dword.dwData == 10000, "got %u expected %u\n", prop_dword.dwData, 10000 );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );

    hr = IDirectInputDevice8_EnumObjects( device, check_objects, &check_objects_params, DIDFT_ALL );
    ok( hr == DI_OK, "EnumObjects returned %#x\n", hr );
    ok( check_objects_params.index >= check_objects_params.expect_count, "missing %u objects\n",
        check_objects_params.expect_count - check_objects_params.index );

    res = 0;
    hr = IDirectInputDevice8_EnumEffects( device, check_effect_count, &res, 0xfe );
    ok( hr == DI_OK, "EnumEffects returned %#x\n", hr );
    ok( res == 0, "got %u expected %u\n", res, 0 );
    res = 0;
    hr = IDirectInputDevice8_EnumEffects( device, check_effect_count, &res, DIEFT_PERIODIC );
    ok( hr == DI_OK, "EnumEffects returned %#x\n", hr );
    ok( res == 2, "got %u expected %u\n", res, 2 );
    hr = IDirectInputDevice8_EnumEffects( device, check_effects, &check_effects_params, DIEFT_ALL );
    ok( hr == DI_OK, "EnumEffects returned %#x\n", hr );
    ok( check_effects_params.index >= check_effects_params.expect_count, "missing %u effects\n",
        check_effects_params.expect_count - check_effects_params.index );

    effectinfo.dwSize = sizeof(DIEFFECTINFOW);
    hr = IDirectInputDevice8_GetEffectInfo( device, &effectinfo, &GUID_Sine );
    ok( hr == DI_OK, "GetEffectInfo returned %#x\n", hr );
    check_member_guid( effectinfo, expect_effects[1], guid );
    check_member( effectinfo, expect_effects[1], "%#x", dwEffType );
    check_member( effectinfo, expect_effects[1], "%#x", dwStaticParams );
    check_member( effectinfo, expect_effects[1], "%#x", dwDynamicParams );
    check_member_wstr( effectinfo, expect_effects[1], tszName );

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned: %#x\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#x\n", hr );

    file = CreateFileW( prop_guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError() );

    hwnd = CreateWindowW( L"static", L"dinput", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, 200, 200,
                          NULL, NULL, NULL, NULL );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#x\n", hr );

    prop_dword.diph.dwHow = DIPH_BYUSAGE;
    prop_dword.diph.dwObj = MAKELONG( HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC );
    prop_dword.dwData = DIPROPAUTOCENTER_ON;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "SetProperty DIPROP_AUTOCENTER returned %#x\n", hr );
    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_AUTOCENTER returned %#x\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "SetProperty DIPROP_FFGAIN returned %#x\n", hr );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#x\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_READONLY, "SetProperty DIPROP_FFLOAD returned %#x\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetForceFeedbackState returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "SendForceFeedbackCommand returned %#x\n", hr );

    escape.dwSize = sizeof(DIEFFESCAPE);
    escape.dwCommand = 0;
    escape.lpvInBuffer = buffer;
    escape.cbInBuffer = 10;
    escape.lpvOutBuffer = buffer + 10;
    escape.cbOutBuffer = 10;
    hr = IDirectInputDevice8_Escape( device, &escape );
    todo_wine
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Escape returned: %#x\n", hr );

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#x\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, &expect_set_device_gain_2, sizeof(expect_set_device_gain_2) );
    prop_dword.dwData = 2000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, &expect_set_device_gain_1, sizeof(expect_set_device_gain_1) );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputDevice8_Escape( device, &escape );
    todo_wine
    ok( hr == DIERR_UNSUPPORTED, "Escape returned: %#x\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    todo_wine
    ok( hr == 0x80040301, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    todo_wine
    ok( hr == 0x80040301, "GetForceFeedbackState returned %#x\n", hr );

    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "SendForceFeedbackCommand returned %#x\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_STOPALL );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_PAUSE );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_CONTINUE );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSON );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSOFF );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#x\n", hr );

    objdata.dwOfs = 0x1e;
    objdata.dwData = 0x80;
    res = 1;
    hr = IDirectInputDevice8_SendDeviceData( device, sizeof(DIDEVICEOBJECTDATA), &objdata, &res, 0 );
    if (version < 0x800) ok( hr == DI_OK, "SendDeviceData returned %#x\n", hr );
    else todo_wine ok( hr == DIERR_INVALIDPARAM, "SendDeviceData returned %#x\n", hr );

    test_periodic_effect( device, file, version );
    test_condition_effect( device, file, version );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %d\n", ref );

    DestroyWindow( hwnd );
    CloseHandle( file );

done:
    pnp_driver_stop();
    cleanup_registry_keys();
    SetCurrentDirectoryW( cwd );
    winetest_pop_context();

    return device != NULL;
}

static void test_device_managed_effect(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_descriptor[] = {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 6),
                INPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    USAGE(1, PID_USAGE_DC_DEVICE_PAUSE),
                    USAGE(1, PID_USAGE_DC_DEVICE_CONTINUE),
                    USAGE(1, PID_USAGE_DC_ENABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_DISABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_STOP_ALL_EFFECTS),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 6),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 6),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Z),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 3),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 4),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(2, 0x00ff),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 0x00008ca0),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 4),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_PARAMETER_BLOCK_OFFSET),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_CP_OFFSET),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_FREE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_POOL_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_RAM_POOL_SIZE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_SIMULTANEOUS_EFFECTS_MAX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEVICE_MANAGED_POOL),
                USAGE(1, PID_USAGE_SHARED_PARAMETER_BLOCKS),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_CREATE_NEW_EFFECT_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_LOAD_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_BLOCK_LOAD_STATUS),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_SUCCESS),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_FULL),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_ERROR),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_RAM_POOL_AVAILABLE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
#include "pop_hid_macros.h"

    static const HIDP_CAPS hid_caps =
    {
        .InputReportByteLength = 5,
    };
    struct hid_expect expect_acquire[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        /* device gain */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 2,
            .report_buf = {6, 0xff},
        },
    };
    struct hid_expect expect_reset[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    struct hid_expect expect_enable_actuators[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x04},
        },
    };
    struct hid_expect expect_disable_actuators[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x05},
        },
    };
    struct hid_expect expect_stop_all[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x06},
        },
    };
    struct hid_expect expect_device_pause[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x02},
        },
    };
    struct hid_expect expect_device_continue[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x03},
        },
    };
    struct hid_expect expect_create[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x01,0x00,0x06,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_2[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x02,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x02,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x02,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x02,0x03,0x08,0x01,0x00,0x06,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_delay[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x01,0x00,0xff,0x7f,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_duration[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x00,0x00,0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_start =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x01, 0x01, 0x01},
    };
    struct hid_expect expect_start_2 =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x02, 0x02, 0x01},
    };
    struct hid_expect expect_stop =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x01, 0x03, 0x00},
    };
    struct hid_expect expect_stop_2 =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x02, 0x03, 0x00},
    };
    struct hid_expect expect_destroy[] =
    {
        /* effect operation */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x01,0x03,0x00},
        },
        /* block free */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {5,0x01},
        },
    };
    struct hid_expect expect_destroy_2[] =
    {
        /* effect operation */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x02,0x03,0x00},
        },
        /* block free */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {5,0x02},
        },
    };
    struct hid_expect device_state_input[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0xff,0x00,0xff},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x12,0x34,0x56,0xff},
        },
    };
    struct hid_expect device_state_input_0[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0xff,0x00,0xff},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x56,0x12,0x34,0xff},
        },
    };
    struct hid_expect device_state_input_1[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x00,0x01,0x01},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x65,0x43,0x21,0x00},
        },
    };
    struct hid_expect device_state_input_2[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x03,0x00,0x01},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x12,0x34,0x56,0xff},
        },
    };
    struct hid_expect expect_pool[] =
    {
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x10,0x00,0x04,0x03},
            .todo = TRUE,
        },
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x10,0x00,0x04,0x03},
            .todo = TRUE,
        },
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] = {
        +3000,
        0,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONDITION expect_condition[3] =
    {
        {
            .lOffset = -500,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = -4000,
            .dwNegativeSaturation = -5000,
            .lDeadBand = 6000,
        },
        {
            .lOffset = 6000,
            .lPositiveCoefficient = 5000,
            .lNegativeCoefficient = -4000,
            .dwPositiveSaturation = 3000,
            .dwNegativeSaturation = 2000,
            .lDeadBand = 1000,
        },
        {
            .lOffset = -7000,
            .lPositiveCoefficient = -8000,
            .lNegativeCoefficient = 9000,
            .dwPositiveSaturation = 10000,
            .dwNegativeSaturation = 11000,
            .lDeadBand = -12000,
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = sizeof(DIEFFECT_DX6),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 2,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = (void *)expect_condition,
        .dwStartDelay = 6000,
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    WCHAR cwd[MAX_PATH], tempdir[MAX_PATH];
    IDirectInputDevice8W *device;
    IDirectInputEffect *effect, *effect2;
    HANDLE file, event;
    ULONG res, ref;
    DIEFFECT desc;
    DWORD flags;
    HRESULT hr;
    HWND hwnd;

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    cleanup_registry_keys();
    if (!dinput_driver_start( report_descriptor, sizeof(report_descriptor), &hid_caps,
                              expect_pool, sizeof(expect_pool) )) goto done;
    if (FAILED(hr = dinput_test_create_device( DIRECTINPUT_VERSION, &devinst, &device ))) goto done;

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#x\n", hr );
    file = CreateFileW( prop_guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError() );

    hwnd = CreateWindowW( L"static", L"dinput", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, 200, 200,
                          NULL, NULL, NULL, NULL );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( event != NULL, "CreateEventW failed, last error %u\n", GetLastError() );
    hr = IDirectInputDevice8_SetEventNotification( device, event );
    ok( hr == DI_OK, "SetEventNotification returned: %#x\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#x\n", hr );
    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned: %#x\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetForceFeedbackState returned %#x\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "SendForceFeedbackCommand returned %#x\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#x\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED | DIGFFS_EMPTY;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#x\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input, sizeof(struct hid_expect) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#x\n", res );
    send_hid_input( file, device_state_input, sizeof(device_state_input) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#x\n", res );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_EMPTY | DIGFFS_ACTUATORSON | DIGFFS_POWERON |
            DIGFFS_SAFETYSWITCHON | DIGFFS_USERFFSWITCHON;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );

    hr = IDirectInputEffect_GetEffectStatus( effect, NULL );
    ok( hr == E_POINTER, "GetEffectStatus returned %#x\n", hr );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );

    flags = DIEP_ALLPARAMS;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#x\n", hr );

    set_hid_expect( file, expect_reset, sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetEffectStatus returned %#x\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#x\n", hr );
    wait_hid_expect( file, 100 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED | DIGFFS_EMPTY;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_create, sizeof(expect_create) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#x\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#x\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_create_2, sizeof(expect_create_2) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &expect_desc, &effect2, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    set_hid_expect( file, &expect_start_2, sizeof(expect_start_2) );
    hr = IDirectInputEffect_Start( effect2, 1, DIES_SOLO );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect2, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, &expect_stop_2, sizeof(expect_stop_2) );
    hr = IDirectInputEffect_Stop( effect2 );
    ok( hr == DI_OK, "Stop returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect2, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_destroy_2, sizeof(expect_destroy_2) );
    ref = IDirectInputEffect_Release( effect2 );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    /* sending commands has no direct effect on status */
    set_hid_expect( file, expect_stop_all, sizeof(expect_stop_all) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_STOPALL );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_device_pause, sizeof(expect_device_pause) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_PAUSE );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_device_continue, sizeof(expect_device_continue) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_CONTINUE );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_disable_actuators, sizeof(expect_disable_actuators) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSOFF );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_enable_actuators, sizeof(expect_enable_actuators) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSON );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_0, sizeof(device_state_input_0) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWERON | DIGFFS_SAFETYSWITCHON | DIGFFS_USERFFSWITCHON;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_1, sizeof(device_state_input_1) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#x\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_ACTUATORSOFF | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_2, sizeof(device_state_input_2) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#x\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#x\n", hr );
    flags = DIGFFS_EMPTY | DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF |
            DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#x\n", res );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );

    /* start delay has no direct effect on effect status */
    desc = expect_desc;
    desc.dwStartDelay = 32767000;
    set_hid_expect( file, expect_create_delay, sizeof(expect_create_delay) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    /* duration has no direct effect on effect status */
    desc = expect_desc;
    desc.dwDuration = 100;
    desc.dwStartDelay = 0;
    set_hid_expect( file, expect_create_duration, sizeof(expect_create_duration) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == 0, "got status %#x\n", res );
    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#x\n", hr );
    set_hid_expect( file, NULL, 0 );
    Sleep( 100 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#x\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#x\n", res );
    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %d\n", ref );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_reset, sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#x\n", hr );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %d\n", ref );

    DestroyWindow( hwnd );
    CloseHandle( event );
    CloseHandle( file );

done:
    pnp_driver_stop();
    cleanup_registry_keys();
    SetCurrentDirectoryW( cwd );
    winetest_pop_context();
}

START_TEST( force_feedback )
{
    if (!dinput_test_init()) return;

    CoInitialize( NULL );
    if (test_force_feedback_joystick( 0x800 ))
    {
        test_force_feedback_joystick( 0x500 );
        test_force_feedback_joystick( 0x700 );
        test_device_managed_effect();
    }
    CoUninitialize();

    dinput_test_exit();
}
